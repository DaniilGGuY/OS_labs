/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include "bakery.h"

bool_t
xdr_REQUEST (XDR *xdrs, REQUEST *objp)
{
	register int32_t *buf;

	 if (!xdr_int (xdrs, &objp->number))
		 return FALSE;
	 if (!xdr_double (xdrs, &objp->arg1))
		 return FALSE;
	 if (!xdr_char (xdrs, &objp->operation))
		 return FALSE;
	 if (!xdr_double (xdrs, &objp->arg2))
		 return FALSE;
	return TRUE;
}
