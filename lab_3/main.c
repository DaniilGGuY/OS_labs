#include <syslog.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <pthread.h>
#include <signal.h>

#define LOCKFILE "/var/run/my_daemon.pid"
#define LOCKMODE (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)

#define TIMEOUT 20

sigset_t mask;

int lockfile(int fd)
{
    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    return fcntl(fd, F_SETLK, &fl);
}

int already_running(void)
{
    int fd;
    char buf[16];

    fd = open(LOCKFILE, O_RDWR|O_CREAT, LOCKMODE);
    if (fd < 0) {
        syslog(LOG_ERR, "невозможно открыть %s: %s", LOCKFILE, strerror(errno));
        exit(1);
    }
    if (lockfile(fd) < 0) {
        if (errno == EACCES || errno == EAGAIN) {
            close(fd);
            return 1;
        }
        syslog(LOG_ERR, "невозможно установить блокировку на %s: %s", LOCKFILE, strerror(errno));
        exit(1);
    }

    ftruncate(fd, 0);
    sprintf(buf, "%ld", (long)getpid());
    write(fd, buf, strlen(buf) + 1);
    return 0;
}

void daemonize(const char *cmd)
{
    int i, fd0, fd1, fd2;
    pid_t pid;
    struct rlimit rl;
    struct sigaction sa;

    /*
    * Сбросить маску режима создания файла.
    */	
    umask(0);
    
    /*
    * Получить максимально возможный номер дескриптора файла.
    */
    if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
        syslog(LOG_ERR, "%s: невозможно получить максимальный номер дескриптора ", cmd);
    
    /*
    * Стать лидером нового сеанса, чтобы утратить управляющий терминал.
    */
    if ((pid = fork()) < 0)
        syslog(LOG_ERR, "%s: ошибка вызова функции fork", cmd);
    else if (pid != 0) /* родительский процесс */
        exit(0);
    setsid();

    /*
    * Обеспечить невозможность обретения управляющего терминала в будущем.
    */
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < 0)
        syslog(LOG_ERR, "%s: невозможно игнорировать сигнал SIGHUP", cmd);

    /*
    * Назначить корневой каталог текущим рабочим каталогом,
    * чтобы впоследствии можно было отмонтировать файловую систему.
    */
    if (chdir("/") < 0)
        printf("%s: невозможно сделать текущим рабочим каталогом", cmd);
    
    /*
    * Закрыть все открытые файловые дескрипторы.
    */
    if (rl.rlim_max == RLIM_INFINITY)
        rl.rlim_max = 1024;
    for (i = 0; i < rl.rlim_max; i++)
        close(i);
    
     /*
    * Присоединить файловые дескрипторы 0, 1 и 2 к /dev/null.
    */
    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup(0);
    fd2 = dup(0);
    
    /*
    * Инициализировать файл журнала.
    */
    openlog(cmd, LOG_CONS, LOG_DAEMON);
    if (fd0 != 0 || fd1 != 1 || fd2 != 2) {
        syslog(LOG_ERR, "ошибочные файловые дескрипторы %d %d %d", fd0, fd1, fd2);
        exit(1);
    }
}

void reread(void) 
{
    FILE* fd;
    char name_buf[128] = "ERROR";
    int uid = -1, pid = -1;

    if ((fd = fopen("/etc/my_daemon.conf", "r")) == NULL)
        syslog(LOG_ERR, "Ошибка открытия файла /etc/daemon.conf");
    else 
    {
        fscanf(fd, "%s %d %d", name_buf, &uid, &pid);
        fclose(fd);
        syslog(LOG_INFO, "User Name: %s.", name_buf);
        syslog(LOG_INFO, "User ID: %d", uid);
        syslog(LOG_INFO, "Process ID: %d", pid);
    }
}

void *thr_fn(void *arg)
{
    int err, signo;

    for (;;) {
        err = sigwait(&mask, &signo);
        if (err != 0) {
            syslog(LOG_ERR, "ошибка вызова функции sigwait");
            exit(1);
        }

        switch (signo) {
            case SIGHUP:
                syslog(LOG_INFO, "получен сигнал SIGHUP; чтение конфигурационного файла");
                reread();
                break;
            case SIGTERM:
                syslog(LOG_INFO, "получен SIGTERM; выход");
                exit(0);   
            default:
                syslog(LOG_INFO, "получен сигнал %d\n", signo);
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int err;
    pthread_t tid;
    char *cmd = "my_daemon";
    struct sigaction sa;

    /*
    * Перейти в режим демона
    */
    daemonize(cmd);
    
    /*
    * Убедиться, что ранее не была запущена другая копия демона
    */
    if (already_running())
    {
        syslog(LOG_ERR, "демон уже запущен");
        exit(1);
    }
    
    FILE* file = fopen("/etc/my_daemon.conf", "w");
    fprintf(file, "%s %d %d", getlogin(), getuid(), getpid());
    fclose(file);

    /*
    * Восстановить действия по умолчанию для сигнала SIGHUP и заблокировать все сигналы
    */
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < 0)
        printf("невозможно восстановить действие SIG_DFL для SIGHUP");
    sigfillset(&mask);
    if ((err = pthread_sigmask(SIG_BLOCK, &mask, NULL)) != 0)
        syslog(LOG_ERR, "ошибка выполнения операции SIG_BLOCK");
    
    /*
    * Создать поток, который будет заниматься обработкой SIGHUP и SIGTERM
    */
    err = pthread_create(&tid, NULL, thr_fn, NULL);
    if (err != 0)
        syslog(LOG_ERR, "невозможно создать поток");

    time_t raw_time;
    struct tm *timeinfo;
    for (;;)
    {
        sleep(TIMEOUT);
        time(&raw_time);
        timeinfo = localtime(&raw_time);
        syslog(LOG_INFO, "Текущее время: %s", asctime(timeinfo));
    }
}