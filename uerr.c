
#include "uerr.h"
#include <string.h>
//#include "../lib/winerr.h"

static struct UERR uerrno; //our errno

void uerr_set( int errclass, long err_no, char *errstr ) // {{{
{
	uerr_init();
	if( uerrno.loaded ) WARN("uerrno is being set even though it is already set. :P");
	uerrno.loaded = 1;
	uerrno.errclass = errclass;
	uerrno.errnumber = err_no;
	vbuf_strset( uerrno.errstr, errstr );
} // }}}

uerr *uerr_get() {
	uerr_init();
	uerrno.loaded = 0;
	return &uerrno;
}

char *uerr_str( uerr *e ) // {{{
{
	static vbuf *vb = NULL;
	if(!vb)vb=vbuf_alloc(0);
	e->loaded = 0;
	uerr_init();
	vbuf_overwrite( vb, uerrno.errstr );
	/*
	switch( e->errclass ) {
		case UERR_UNIX:
			vbuf_strset( vb, strerror( e->errno ) );
			break;
		case UERR_WIN:
			{
				void *buf = NULL;
				int len;
				len = rlFormatMessage(
					rlFORMAT_MESSAGE_FROM_SYSTEM | rlFORMAT_MESSAGE_ALLOCATE_BUFFER,
					NULL, //I guess zero is valid here...? :P
					uerrno.errno,
					0,
					(LPTSTR)&buf,
					0,
					NULL );
				vbuf_strset( vb, buf );
			}
			break;
		case UERR_MINE:
			DIE("You need to write me.");
			break;
		default:
			DIE("Unknown error class.");
	}
	*/

	if( uerrno.vbstr->dlen > 1 ) {
		vbuf_strncat(vb, ": ", 2);
		if( uerrno.vbstr )
			vbuf_strncat(vb, uerrno.vbstr->b, uerrno.vbstr->dlen );
	}

	return vb->b;
} // }}}

void uerr_init() // {{{
{
	if(!uerrno.errstr ) {
		uerrno.errstr = vbuf_alloc(100);
		vbuf_strset( uerrno.errstr, "Error not set" );
	}
	if(!uerrno.vbstr ) {
		uerrno.vbstr = vbuf_alloc(100);
		vbuf_strset( uerrno.vbstr, "" );
	}
} // }}}
