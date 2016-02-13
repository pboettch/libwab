/*
 * uerr.h
 *
 * This is what I am doing to deal with errors.  I needed a way to be able to
 * express unix, win32 and my own errors.
 *
 * I'm not sure I like it, but here it is.
 *
 */
#ifndef UERR_H
#define UERR_H

#define UERR_UNIX 0
#define UERR_WIN 1
#define UERR_MINE 2

#include "tools.h"
#include <unistd.h>

typedef struct UERR {
	int errclass; //UERR_UNIX, UERR_WIN, UERR_MINE
	long errnumber;
	vbuf *errstr; //stores the error message from the system
	vbuf *vbstr;  //stores an arbitrary message
	int loaded; // 1 == error message is waiting to be used, 0 otherwise
} uerr;

#define eprintf( eclass, errno, errstr, ... ) { uerr_init(); uerr_set( eclass, errno, errstr ); vbuf_printf(uerr_get()->vbstr, __VA_ARGS__ ); }
#define ERR_UNIX( anerrno, ... ) eprintf( UERR_UNIX, anerrno, strerror( anerrno ), __VA_ARGS__ ); 
//#define ERR_WIN( anerrno, ... ) eprintf( UERR_WIN, anerrno, strerror( anerrno ), __VA_ARGS );
void uerr_set( int errclass, long err_no, char *errstr );
uerr *uerr_get();
char *uerr_str( uerr *e );
void uerr_init();

#endif
