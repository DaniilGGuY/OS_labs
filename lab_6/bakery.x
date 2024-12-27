struct REQUEST
{
    int number;
    double arg1;
    char operation;
    double arg2;
};

program BAKERY_PROG
{
    version BAKERY_VER
    {
        int GET_NUMBER() = 1;
        double BAKERY_SERVICE(struct REQUEST) = 2;
    } = 1; /* Version number = 1 */
} = 0x20000001; /* RPC program number */