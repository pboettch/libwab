// {{{ includes

#include <ctype.h>
//#include "defines.h"
#include <errno.h>
#include <iconv.h>
#include <malloc.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
//#include "../misc/globals.h"
#include "tools.h"
#include "uerr.h"

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

#ifndef index
#define index strchr
#endif

// }}}

// {{{ Macros: ASSERT(), DIE(), F_MALLOC()
void pDIE( char *fmt, ... ) // {{{ Cough...cough
{
	va_list ap;
	va_start( ap, fmt );
        //fprintf( stderr, "Fatal error (will segfault): ");
	vfprintf( stderr, fmt, ap );
	fprintf( stderr, "\n" );
	va_end(ap);
        raise( SIGSEGV );
}
// }}}
void pWARN( char *fmt, ... ) // {{{ Cough...cough
{
	va_list ap;
	va_start( ap, fmt );
        fprintf( stderr, "WARNING: ");
	vfprintf( stderr, fmt, ap );
	fprintf( stderr, "\n" );
	va_end(ap);
}
// }}}
void *F_MALLOC( size_t size ) // {{{ malloc() but dumps core when it fails
{
	void *result;

	result = malloc( size );
	ASSERT( NULL != result, "malloc() failure." );

	return result;
}
// }}}
void *F_REALLOC( void *p, size_t size ) // {{{ realloc() but dumps core when it fails
{
	void *result;

	//if( NULL != p ) hexdump((char*)p - 128, 0, 128, 1 );
	if(!p) {
		ASSERT( NULL != ( result = malloc( size ) ), "malloc() failure." );
	}
	else {
		ASSERT( NULL != ( result = realloc( p, size ) ), "realloc() failure." );
	}

	//hexdump((char*)result - 128, 0, 128, 1 );
	fflush(stderr);
	return result;
}
// }}}
// }}}
// {{{ Program logging/debug output
int DEBUG_LEVEL = DB_INFO;

void db_default( char *file, int line, int level, char *fmt, ... ) // {{{
{
	va_list ap;
	if( level <= DEBUG_LEVEL ) {
		switch( level ) {
			case DB_CRASH:
				fprintf(stderr, "CRASH");
				break;
			case DB_ERR:
				fprintf(stderr, "ERROR");
				break;
			case DB_WARN:
				fprintf(stderr, "WARNING");
				break;
			case DB_INFO:
			case DB_VERB:
				break;
			default:
				fprintf(stderr, "DEBUG(%d)", level );
		}

		if( level <= DB_WARN )
			fprintf(stderr, " (%s:%d)", file, line );

		if( DB_INFO != level && DB_VERB != level )
			fprintf(stderr, ": ");

		va_start( ap, fmt );
		vfprintf(stderr, fmt, ap );
		fprintf(stderr, "\n" );
		va_end( ap );
	}
} // }}}

void (*dbfunc)(char *file, int line, int level, char *fmt, ...) = &db_default;

//#define DEBUG(x) { x; }
//#define DEBUG(x) ;
// }}}
// {{{ UTF8 <-> UTF16 <-> ISO8859 Character set conversion functions and (ack) their globals

//TODO: the following should not be
char *wwbuf=NULL;
size_t nwwbuf=0;
static int unicode_up=0;
//iconv_t i16to8, i8to16, i8859_1to8, i8toi8859_1;
iconv_t i16to8;

void unicode_init() // {{{
{
	//char *wipe = "";
	//char dump[4];
	VBUF_STATIC( dump, 4 );

	char preamp[] = { 0xff, 0xfe, 'm', 0, 'e', 0, 'o', 0, 'w', 0, 0, 0 };

	char preamp2[] = { 'm', 0, 'e', 0, 'o', 0, 'w', 0, 0, 0 };

	if( unicode_up ) unicode_close();

	if( (iconv_t)-1 == (i16to8 = iconv_open( "UTF-8", "UTF-16" ) ) ) {
		fprintf(stderr, "doexport(): Couldn't open iconv descriptor for UTF-16 to UTF-8.\n");
		exit( 1 );
	}

	vb_utf16to8( dump, preamp, sizeof( preamp ) );
	//fprintf(stderr, "Initial iconv: %s\n", dump->b );

	vb_utf16to8( dump, preamp2, sizeof( preamp ) );
	//fprintf(stderr, "Initial iconv2: %s\n", dump->b );

	/*
	if( (iconv_t)-1 == (i8to16 = iconv_open( "UTF-16", "UTF-8" ) ) ) {
		fprintf(stderr, "doexport(): Couldn't open iconv descriptor for UTF-8 to UTF-16.\n");
		exit( 2 );
	}
	*/

	//iconv will prefix output with an FF FE (utf-16 start seq), the following dumps that.
	/*
	memset( dump, 'x', 4 );
	ASSERT( 0 == utf8to16( wipe, 1, dump, 4 ), "unicode_init(): attempt to dump FF FE failed." );
	*/

	/*
	if( (iconv_t)-1 == (i8859_1to8 = iconv_open( "UTF-8", "ISO_8859-1" ) ) ) {
		fprintf(stderr, "doexport(): Couldn't open iconv descriptor for ASCII to UTF-8.\n");
		exit( 1 );
	}


	if( (iconv_t)-1 == (i8toi8859_1 = iconv_open( "ISO_8859-1", "UTF-8" ) ) ) {
		fprintf(stderr, "doexport(): Couldn't open iconv descriptor for UTF-8 to ASCII.\n");
		exit( 1 );
	}
	*/
}
// }}}
void unicode_close() // {{{
{
	unicode_up = 0;
	//iconv_close( i8to16 );
	iconv_close( i16to8 );
	//iconv_close( i8859_1to8 );
	//iconv_close( i8toi8859_1 );
}
// }}}

char *utf16buf = NULL;
int utf16buf_len = 0;

//int utf16to8( char *inbuf_o, char *outbuf_o, int length ) // {{{
//{
//	int inbytesleft = length;
//	int outbytesleft = length;
//	char *inbuf = inbuf_o;
//	char *outbuf = outbuf_o;
//	int rlen = -1, tlen;
//	int icresult = -1; 
//
//	int i, strlen=-1;
//	
//	DEBUG(
//		fprintf(stderr, "  utf16to8(): attempting to convert:\n");
//		//hexdump( (char*)inbuf_o, 0, length, 1 );
//		fflush(stderr);
//	);
//
//	for( i=0; i<length ; i+=2 ) {
//		if( inbuf_o[i] == 0 && inbuf_o[i + 1] == 0 ) {
//			//fprintf(stderr, "End of string found at: %d\n", i );
//			strlen = i;
//		}
//	}
//
//	//hexdump( (char*)inbuf_o, 0, strlen, 1 );
//
//	if( -1 == strlen ) WARN("String is not zero-terminated.");
//
//	//iconv does not like it when the inbytesleft > actual string length
//	//enum: zero terminated, length valid
//	//      zero terminated, length short //we won't go beyond length ever, so this is same as NZT case
//	//      zero terminated, length long
//	//      not zero terminated
//	//      TODO: MEMORY BUG HERE!
//	for( tlen = 0; tlen <= inbytesleft - 2; tlen+=2 ) {
//		if( inbuf_o[tlen] == 0 && inbuf_o[tlen+1] == 0 ){
//			rlen = tlen + 2;
//			tlen = rlen;
//			break;
//		}
//		if( tlen == inbytesleft )fprintf(stderr, "Space allocated for string > actual string length.  Go windows!\n");
//	}
//
//	if( rlen >= 0 )
//		icresult = iconv( i16to8, &inbuf, &rlen, &outbuf, &outbytesleft );
//
//	if( icresult == (size_t)-1 ) {
//		fprintf(stderr, "utf16to8(): iconv failure(%d): %s\n", errno, strerror( errno ) );
//		fprintf(stderr, "  attempted to convert:\n");
//		hexdump( (char*)inbuf_o, 0, length, 1 );
//		fprintf(stderr, "  result:\n");
//		hexdump( (char*)outbuf_o, 0, length, 1 );
//		fprintf(stderr, "  MyDirtyOut:\n");
//		for( i=0; i<length; i++) {
//			if( inbuf_o[i] != '\0' ) fprintf(stderr, "%c", inbuf_o[i] );
//		}
//
//		fprintf( stderr, "\n" );
//		raise( SIGSEGV );
//		exit(1);
//	}
//
//	DEBUG(
//		fprintf(stderr, "  result:\n");
//		hexdump( (char*)outbuf_o, 0, length, 1 );
//	     )
//
//	//fprintf(stderr, "utf16to8() returning %s\n", outbuf );
//
//	return icresult;	
//}
//// }}}
int utf16_is_terminated( char *str, int length ) // {{{
{
	VBUF_STATIC( errbuf, 100 );
	int len = -1;
	int i;
	for( i=0; i<length ; i+=2 ) {
		if( str[i] == 0 && str[i + 1] == 0 ) {
			//fprintf(stderr, "End of string found at: %d\n", i );
			len = i;
		}
	}

	//hexdump( (char*)inbuf_o, 0, len, 1 );

	if( -1 == len ) {
		vbuf_hexdump( errbuf, str, 0, length, 1 );
		WARN("String is not zero terminated (probably broken data from registry) %s.", errbuf->b);
	}

	return (-1 == len )?0:1;
} // }}}
int vb_utf16to8( vbuf *dest, char *buf, size_t len ) // {{{
{
	size_t inbytesleft = len;
	char *inbuf = buf;
	//int rlen = -1, tlen;
	int icresult = -1; 
	VBUF_STATIC( dumpster, 100 );

	//int i; //, strlen=-1;
	size_t outbytesleft; 
	char *outbuf;

	if( 2 > dest->blen ) vbuf_resize( dest, 2 );
	dest->dlen = 0;

	//Bad Things can happen if a non-zero-terminated utf16 string comes through here
	if( !utf16_is_terminated( buf, len ) ) return -1;

	do {
		outbytesleft = dest->blen - dest->dlen;
		outbuf = dest->b + dest->dlen;
		icresult = iconv( i16to8, &inbuf, &inbytesleft, &outbuf, &outbytesleft );
		dest->dlen = outbuf - dest->b;
		vbuf_grow( dest, inbytesleft);
	} while( (size_t)-1 == icresult && E2BIG == errno );

	/*
	if( 0 != vb_utf8to16T( dumpster, dest->b, dest->dlen ) )
		DIE("Reverse conversion failed.");
		*/

	if( icresult == (size_t)-1 ) {
		//TODO: error
		ERR_UNIX( errno, "vb_utf16to8():iconv failure: %s", strerror( errno ) );
		unicode_init();
		return -1;
		/*
		fprintf(stderr, "  attempted to convert:\n");
		hexdump( (char*)cin, 0, inlen, 1 );
		fprintf(stderr, "  result:\n");
		hexdump( (char*)bout->b, 0, bout->dlen, 1 );
		fprintf(stderr, "  MyDirtyOut:\n");
		for( i=0; i<inlen; i++) {
			if( inbuf[i] != '\0' ) fprintf(stderr, "%c", inbuf[i] );
		}

		fprintf( stderr, "\n" );
		raise( SIGSEGV );
		exit(1);
		*/
	}

	if( icresult ) {
		ERR_UNIX( EILSEQ, "Uhhhh...vb_utf16to8() returning icresult == %d", icresult );
		return -1;
	}
	return icresult;	
}
// }}}

//int utf8to16( char *inbuf_o, int iblen, char *outbuf_o, int oblen) // {{{ iblen, oblen: bytes including \0
//{
//	//TODO: This (and 8to16) are the most horrible things I have ever seen...
//	int inbytesleft;
//	int outbytesleft = oblen;
//	char *inbuf = inbuf_o;
//	char *outbuf = outbuf_o;
//	//int rlen = -1, tlen;
//	int icresult = -1; 
//
//	char *stend;
//
//	int i; //, strlen=-1;
//	
//	DEBUG(
//		fprintf(stderr, "  utf8to16(): attempting to convert:\n");
//		//hexdump( (char*)inbuf_o, 0, length, 1 );
//		fflush(stderr);
//	);
//
//	stend = memchr( inbuf_o, '\0', iblen );
//	ASSERT( NULL != stend, "utf8to16(): in string not zero terminated." );
//
//	inbytesleft = ( stend - inbuf_o + 1 < iblen )? stend - inbuf_o + 1: iblen;
//
//	//iconv does not like it when the inbytesleft > actual string length
//	//enum: zero terminated, length valid
//	//      zero terminated, length short //we won't go beyond length ever, so this is same as NZT case
//	//      zero terminated, length long
//	//      not zero terminated
//	//      TODO: MEMORY BUG HERE!
//	//      
//	/*
//	for( tlen = 0; tlen <= inbytesleft - 2; tlen+=2 ) {
//		if( inbuf_o[tlen] == 0 && inbuf_o[tlen+1] == 0 ){
//			rlen = tlen + 2;
//			tlen = rlen;
//			break;
//		}
//		if( tlen == inbytesleft )fprintf(stderr, "Space allocated for string > actual string length.  Go windows!\n");
//	}
//	*/
//
//	//if( rlen >= 0 )
//		icresult = iconv( i8to16, &inbuf, &inbytesleft, &outbuf, &outbytesleft );
//
//	if( icresult == (size_t)-1 ) {
//		fprintf(stderr, "utf8to16(): iconv failure(%d): %s\n", errno, strerror( errno ) );
//		fprintf(stderr, "  attempted to convert:\n");
//		hexdump( (char*)inbuf_o, 0, iblen, 1 );
//		fprintf(stderr, "  result:\n");
//		hexdump( (char*)outbuf_o, 0, oblen, 1 );
//		fprintf(stderr, "  MyDirtyOut:\n");
//		for( i=0; i<iblen; i++) {
//			if( inbuf_o[i] != '\0' ) fprintf(stderr, "%c", inbuf_o[i] );
//		}
//
//		fprintf( stderr, "\n" );
//		raise( SIGSEGV );
//		exit(1);
//	}
//
//	DEBUG(
//		fprintf(stderr, "  result:\n");
//		hexdump( (char*)outbuf_o, 0, oblen, 1 );
//	     )
//
//	//fprintf(stderr, "utf8to16() returning %s\n", outbuf );
//
//	//TODO: error
//	if( icresult ) printf("Uhhhh...utf8to16() returning icresult == %d\n", icresult );
//	return icresult;	
//}
//// }}}
//int iso8859_1to8( char *inbuf_o, char *outbuf_o, int length ) // {{{
//{
//	int outbytesleft = length;
//	char *inbuf = inbuf_o;
//	char *outbuf = outbuf_o;
//	int rlen = -1;
//	int icresult = -1; 
//	
//	DEBUG(
//		fprintf(stderr, "  iso8859_1to8(): attempting to convert:\n");
//		hexdump( (char*)inbuf_o, 0, length, 1 );
//	);
//
//	//iconv does not like it when the inbytesleft > actual string length
//	//enum: zero terminated, length valid
//	//      zero terminated, length short //we won't go beyond length ever, so this is same as NZT case
//	//      zero terminated, length long
//	//      not zero terminated
//
//	rlen = strlen( inbuf_o );
//	DEBUG(
//	if( rlen != length )
//		fprintf(stderr, "iso8859_1to8(): string length mismatch, %d (given) != %d (strlen)\n", length, rlen );
//	     );
//
//	if( rlen >= 0 )
//		icresult = iconv( i8859_1to8, &inbuf, &rlen, &outbuf, &outbytesleft );
//
//	if( icresult == (size_t)-1 ) {
//		fprintf(stderr, "iso8859_1to8(): iconv failure: %s\n", strerror( errno ) );
//		fprintf(stderr, "  attempted to convert:\n");
//		hexdump( (char*)inbuf_o, 0, length, 1 );
//		fprintf(stderr, "  result:\n");
//		hexdump( (char*)outbuf_o, 0, length, 1 );
//		raise( SIGSEGV );
//		exit(1);
//	}
//
//	DEBUG(
//		fprintf(stderr, "  result:\n");
//		hexdump( (char*)outbuf_o, 0, length, 1 );
//	     );
//
//	//fprintf(stderr, "utf16to8() returning %s\n", outbuf );
//
//	//return icresult;	
//
//	return length - outbytesleft;
//}
//// }}}
//int utf8toascii( const char *inbuf_o, char *outbuf_o, int length ) // {{{
//{
//	int outbytesleft = length;
//	char *inbuf = (char *)inbuf_o; //TODO: so what's with the const if we do this?
//	char *outbuf = outbuf_o;
//	int rlen = -1;
//	int icresult = -1; 
//	
//	DEBUG(
//		fprintf(stderr, "  utf8toascii(): attempting to convert:\n");
//		hexdump( (char*)inbuf_o, 0, length, 1 );
//	);
//
//	//iconv does not like it when the inbytesleft > actual string length
//	//enum: zero terminated, length valid
//	//      zero terminated, length short //we won't go beyond length ever, so this is same as NZT case
//	//      zero terminated, length long
//	//      not zero terminated
//
//	rlen = strlen( inbuf_o );
//	DEBUG(
//	if( rlen != length )
//		fprintf(stderr, "utf8toascii(): string length mismatch, %d (given) != %d (strlen)\n", length, rlen );
//	     );
//
//	if( rlen >= 0 )
//		icresult = iconv( i8toi8859_1, &inbuf, &rlen, &outbuf, &outbytesleft );
//
//	if( icresult == (size_t)-1 ) {
//		fprintf(stderr, "utf8toascii(): iconv failure: %s\n", strerror( errno ) );
//		fprintf(stderr, "  attempted to convert:\n");
//		hexdump( (char*)inbuf_o, 0, length, 1 );
//		fprintf(stderr, "  result:\n");
//		hexdump( (char*)outbuf_o, 0, length, 1 );
//		raise( SIGSEGV );
//		exit(1);
//	}
//
//	DEBUG(
//		fprintf(stderr, "  result:\n");
//		hexdump( (char*)outbuf_o, 0, length, 1 );
//	     );
//
//	//fprintf(stderr, "utf16to8() returning %s\n", outbuf );
//
//	//return icresult;	
//
//	return length - outbytesleft;
//}
//// }}}

#if 0
void cheap_uni2ascii(char *src, char *dest, int l) /* {{{ Quick and dirty UNICODE to std. ascii */
{

	for (; l > 0; l -=2) {
		*dest = *src;
		dest++; src +=2;
	}
	*dest = 0;
}
// }}}
#endif

void cheap_ascii2uni(char *src, char *dest, int l) /* {{{ Quick and dirty ascii to unicode */
{
   for (; l > 0; l--) {
      *dest++ = *src++;
      *dest++ = 0;
   }
}
// }}}

// }}}
// {{{ VARBUF Functions 
struct varbuf *vbuf_alloc( size_t len ) // {{{
{
	struct varbuf *result;

	result = F_MALLOC( sizeof( struct varbuf ) );

	result->dlen = 0;
	result->blen = 0;
	result->buf = NULL;

	vbuf_resize( result, len );

	return result;

} // }}}
void vbuf_check( vbuf *vb ) // {{{
{
	ASSERT( vb->b - vb->buf <= vb->blen, "vbuf_check(): vb->b outside of buffer range.");
	ASSERT( vb->dlen <= vb->blen, "vbuf_check(): data length > buffer length.");

	ASSERT( vb->blen < 1024*1024, "vbuf_check(): blen is a bit large...hmmm.");
} // }}}
void vbuf_free( vbuf *vb ) // {{{
{
	free( vb->buf );
	free( vb );
} // }}}
void vbuf_clear( struct varbuf *vb ) // {{{ditch the data, keep the buffer
{
	vbuf_resize( vb, 0 );
} // }}}
void vbuf_resize( struct varbuf *vb, size_t len ) // {{{ DESTRUCTIVELY grow or shrink buffer
{
	vb->dlen = 0;

	if( vb->blen >= len ) {
		vb->b = vb->buf;
		return;
	}

	vb->buf = F_REALLOC( vb->buf, len );
	vb->b = vb->buf;
	vb->blen = len;
} // }}}
int vbuf_avail( vbuf *vb ) // {{{
{
	return vb->blen - ((char*)vb->b - (char*)vb->buf + vb->dlen);
} // }}}
void vbuf_dump( vbuf *vb ) // {{{ TODO: to stdout?  Yuck
{
	printf("vb dump-------------\n");
    printf("dlen: %zd\n", vb->dlen );
	printf("blen: %zd\n", vb->blen );
	printf("b - buf: %zd\n", vb->b - vb->buf );
	printf("buf:\n");
	hexdump( vb->buf, 0, vb->blen, 1 );
	printf("b:\n");
	hexdump( vb->b, 0, vb->dlen, 1 );
	printf("^^^^^^^^^^^^^^^^^^^^\n");
} // }}}
void vbuf_grow( struct varbuf *vb, size_t len ) // {{{ out: vbuf_avail(vb) >= len, data are preserved
{
	if( 0 == len ) return;

	if( 0 == vb->blen ) {
		vbuf_resize( vb, len );
		return;
	}

	if( vb->dlen + len > vb->blen ) {
		if( vb->dlen + len < vb->blen * 1.5 ) len = vb->blen * 1.5;
		char *nb = F_MALLOC( vb->blen + len );
		//printf("vbuf_grow() got %p back from malloc(%d)\n", nb, vb->blen + len);
		vb->blen = vb->blen + len;
		memcpy( nb, vb->b, vb->dlen );

		//printf("vbuf_grow() I am going to free %p\n", vb->buf );
		free( vb->buf );
		vb->buf = nb;
		vb->b = vb->buf;
	} else {
		if( vb->b != vb->buf ) 
			memcpy( vb->buf, vb->b, vb->dlen );
	}

	vb->b = vb->buf;

	ASSERT( vbuf_avail( vb ) >= len, "vbuf_grow(): I have failed in my mission." );
} // }}}
void vbuf_strset( struct varbuf *vb, char *s ) // {{{ Store string s in vb
{
	vbuf_strnset( vb, s, strlen( s ) );
} // }}}
void vbuf_strnset( struct varbuf *vb, char *s, int n ) // {{{ Store string s in vb
{
	vbuf_resize( vb, n + 1 );
	memcpy( vb->b, s, n);
	vb->b[n] = '\0';
	vb->dlen = n+1;
} // }}}
void vbuf_strnset16( vbuf *vb, char *s, size_t len ) // {{{ Like vbuf_strnset, but for UTF16
{
	vbuf_resize( vb, len+1 );
	memcpy( vb->b, s, len );
	
	vb->b[len] = '\0';
	vb->dlen = len+1;
	vb->b[len] = '\0';
} // }}}
void vbuf_set( vbuf *vb, void *b, size_t len ) // {{{ set vbuf b size=len, resize if necessary, relen = how much to over-allocate
{
	vbuf_resize( vb, len );

	memcpy( vb->b, b, len );
	vb->dlen = len;
} // }}}
void vbuf_skipws( struct varbuf *vb ) // {{{
{
	char *p = vb->b;
	while( p - vb->b < vb->dlen && isspace( p[0] ) ) p++;

	vbuf_skip( vb, p - vb->b );
} // }}}
void vbuf_append( struct varbuf *vb, void *b, size_t len ) // {{{ append len bytes of b to vbuf, resize if necessary
{
	if( 0 == vb->dlen ) {
		vbuf_set( vb, b, len );
		return;
	}

	vbuf_grow( vb, len );

	memcpy( vb->b + vb->dlen, b, len );
	vb->dlen += len;

	//printf("vbuf_append() end: >%s/%d<\n", vbuf->b, vbuf->dlen );
} // }}}
void vbuf_skip( struct varbuf *vb, size_t skip ) // {{{ dumps the first skip characters from vbuf
{
	ASSERT( skip <= vb->dlen, "vbuf_skip(): Attempt to seek past end of buffer." );
	//memmove( vbuf->b, vbuf->b + skip, vbuf->dlen - skip );
	vb->b += skip;
	vb->dlen -= skip;
} // }}}
void vbuf_overwrite( struct varbuf *vbdest, struct varbuf *vbsrc ) // {{{ overwrite vbdest with vbsrc
{
	vbuf_resize( vbdest, vbsrc->blen );
	memcpy( vbdest->b, vbsrc->b, vbsrc->dlen );
	vbdest->blen = vbsrc->blen;
	vbdest->dlen = vbsrc->dlen;
} // }}}
void vbuf_strcat( vbuf *vb, char *str ) // {{{
{
	vbuf_strncat( vb, str, strlen(str ) );
} // }}}
void vbuf_strncat( struct varbuf *vb, char *str, int len ) // {{{ append string str to vbuf, vbuf must already contain a valid string 
{
	ASSERT( vb->b[vb->dlen-1] == '\0', "vbuf_strncat(): attempt to append string to non-string.");
	int sl = strlen( str );
	int n = (sl<len)?sl:len;
	//string append
	vbuf_grow( vb, n + 1 );
	memcpy( vb->b + vb->dlen - 1, str, n );
	//strncat( vb->b, str, n );

	vb->dlen += n;
	vb->b[ vb->dlen - 1 ] = '\0';
} // }}}
void vbuf_charcat( struct varbuf *vb, int ch ) // {{{
{
	vbuf_grow(vb,1);
	vb->b[vb->dlen-1] = ch;
	vb->b[vb->dlen] = '\0';
	vb->dlen++;
} // }}}
void vbuf_strnprepend( struct varbuf *vb, char *str, int len ) // {{{ prependappend string str to vbuf, vbuf must already contain a valid string 
{
	ASSERT( vb->b[vb->dlen-1] == '\0', "vbuf_strncat(): attempt to append string to non-string.");
	int sl = strlen( str );
	int n = (sl<len)?sl:len;
	//string append
	vbuf_grow( vb, n + 1 );
	memmove( vb->b + n, vb->b, vb->dlen - 1 );
	memcpy( vb->b, str, n );
	//strncat( vb->b, str, n );

	vb->dlen += n;
	vb->b[ vb->dlen - 1 ] = '\0';
} // }}}
int vb_skipline( struct varbuf *vb ) // {{{ in: vb->b == "stuff\nmore_stuff"; out: vb->b == "more_stuff"
{
	int nloff = find_nl( vb );
	int nll = skip_nl( vb->b + nloff );

	if( nloff < 0 ) {
		//TODO: error
		printf("vb_skipline(): there seems to be no newline here.\n");
		return -1;
	}
	if( skip_nl < 0 ) {
		//TODO: error
		printf("vb_skipline(): there seems to be no newline here...except there should be. :P\n");
		return -1;
	}

	memmove( vb->b, vb->b + nloff + nll, vb->dlen - nloff - nll );

	vb->dlen -= nloff + nll;

	return 0;
} // }}}
int vbuf_catprintf( vbuf *vb, char *fmt, ... ) // {{{
{
	int size;
	va_list ap;

	/* Guess we need no more than 100 bytes. */
	//vbuf_resize( vb, 100 );
	if(!vb->b || vb->dlen == 0) {
		vbuf_strset( vb, "" );
	}

	while (1) {
		/* Try to print in the allocated space. */
		va_start(ap, fmt);
		size = vsnprintf (vb->b + vb->dlen - 1, vb->blen - vb->dlen, fmt, ap);
		va_end(ap);

		/* If that worked, return the string. */
		if (size > -1 && size < vb->blen - vb->dlen ) {
			vb->dlen += size;
			return size;
		}
		/* Else try again with more space. */
		if ( size >= 0 )    /* glibc 2.1 */
			vbuf_grow( vb, size+1 ); /* precisely what is needed */
		else           /* glibc 2.0 */
			vbuf_grow( vb, vb->blen);
	}
} // }}}
int vs_last( vbuf *vb ) // {{{ returns the last character stored in a vbuf string
{
	if( vb->dlen < 1 ) return -1;
	if( vb->b[vb->dlen-1] != '\0' ) return -1;
	if( vb->dlen == 1 ) return '\0';
	return vb->b[vb->dlen-2];
} // }}}
void vbuf_printf( vbuf *vb, char *fmt, ... ) // {{{ print over vb
{
	int size;
	va_list ap;

	/* Guess we need no more than 100 bytes. */
	vbuf_resize( vb, 100 );

	while (1) {
		/* Try to print in the allocated space. */
		va_start(ap, fmt);
		size = vsnprintf (vb->b, vb->blen, fmt, ap);
		va_end(ap);

		/* If that worked, return the string. */
		if (size > -1 && size < vb->blen) {
			vb->dlen = size + 1;
			return;
		}
		/* Else try again with more space. */
		if ( size >= 0 )    /* glibc 2.1 */
			vbuf_resize( vb, size+1 ); /* precisely what is needed */
		else           /* glibc 2.0 */
			vbuf_resize( vb, vb->blen*2);
	}
} // }}}
void vbuf_printfa( vbuf *vb, char *fmt, ... ) // {{{ printf append to vb
{
	int size;
	va_list ap;

	if( vb->blen - vb->dlen < 50 )
		vbuf_grow( vb, 100 );

	while (1) {
		/* Try to print in the allocated space. */
		va_start(ap, fmt);
		size = vsnprintf (vb->b + vb->dlen - 1, vb->blen - vb->dlen + 1, fmt, ap);
		va_end(ap);

		/* If that worked, return the string. */
		if (size > -1 && size < vb->blen) {
			vb->dlen += size;
			return;
		}
		/* Else try again with more space. */
		if ( size >= 0 )    /* glibc 2.1 */
			vbuf_grow( vb, size+1 - vb->dlen ); /* precisely what is needed */
		else           /* glibc 2.0 */
			vbuf_grow( vb, size );
	}
} // }}}
void vbuf_hexdump( vbuf *vb, char *b, int start, int stop, int ascii ) // {{{
{
	char c;
	int diff,i;

	vbuf_strset( vb, "" );

	while (start < stop ) {
		diff = stop - start;
		if (diff > 16) diff = 16;

		vbuf_printfa(vb, ":%08X  ",start);

		for (i = 0; i < diff; i++) {
			if( 8 == i ) vbuf_printfa( vb, " " );
			vbuf_printfa(vb, "%02X ",(unsigned char)*(b+start+i));
		}
		if (ascii) {
			for (i = diff; i < 16; i++) vbuf_printfa(vb, "   ");
			for (i = 0; i < diff; i++) {
				c = *(b+start+i);
				vbuf_printfa(vb, "%c", isprint(c) ? c : '.');
			}
		}
		vbuf_printfa(vb, "\n");
		start += 16;
	}
} // }}}
void vbuf_strtrunc( vbuf *v, int off ) // {{{ Drop chars [off..dlen] 
{
	if( off >= v->dlen - 1 ) return; //nothing to do
	v->b[off] = '\0';
	v->dlen = off + 1;
}
// }}}
// {{{ User input
// TODO: not sure how useful this stuff is here
int fmyinput(char *prmpt, char *ibuf, int maxlen) /* {{{ get user input */
{
	printf("%s",prmpt);

	fgets(ibuf,maxlen+1,stdin);

	ibuf[strlen(ibuf)-1] = 0;

	return(strlen(ibuf));
}
// }}}
int gethex(char **c) /* {{{ return the value of an ascii hex string */
{
	int value;

	skipspace(c);
	if (!(**c)) return(0);
	sscanf(*c,"%x",&value);
	while( **c != ' ' && (**c)) (*c)++;
	return(value);
}
// }}}
int gethexorstr(char **c, char *wb) /* {{{ Get a string of HEX bytes (space separated), or if first char is ' get an ASCII string instead.  */
{
	int l = 0;

	skipspace(c);

	if ( **c == '\'') {
		(*c)++;
		while ( **c ) {
			*(wb++) = *((*c)++);
			l++;
		}
	} else {
		do {
			*(wb++) = gethex(c);
			l++;
			skipspace(c);
		} while ( **c );
	}
	return(l);
}
// }}}
//}}}
//{{{ String formatting and output to FILE *stream or just stdout, etc 
// TODO: a lot of old, unused stuff in here
void hexprnt(FILE *stream, unsigned char *bytes, int len) /* {{{ Print len number of hexbytes */
{
	int i;
	//printf("%s",s);
	for (i = 0; i < len; i++) {
		fprintf(stream, "%02x ",bytes[i]);
	}
}
// }}}
void vbwinhex8(vbuf *vb, unsigned char *hbuf, int start, int stop, int loff ) // {{{ Produce regedit-style hex output */
{
	int i;
	int lineflag=0;

	for( i=start; i<stop; i++)
	{
		loff += vbuf_catprintf( vb, "%02x", hbuf[i] );
		if( i < stop - 1 ) {
			loff+=vbuf_catprintf( vb, "," );
			switch( lineflag ) {
				case 0:
				if( loff >= 77) {
					lineflag=1;
					loff=0;
					vbuf_catprintf( vb, "\\%s  ", stupid_cr );
				}
				break;
				case 1:
				if( loff >= 75 ) {
					loff=0;
					vbuf_catprintf( vb, "\\%s  ", stupid_cr );
				}
				break;
			}
			//	if( 24 < i || 0 == (i - 17) % 25 ) fprintf( stream, "\\\n  " );
		}
	}

	//   fprintf( stream, "\n" );
} // }}}
void winhex8(FILE *stream, unsigned char *hbuf, int start, int stop, int loff ) // {{{ Produce regedit-style hex output */
{
	int i;
	int lineflag=0;

	for( i=start; i<stop; i++)
	{
		loff += fprintf( stream, "%02x", hbuf[i] );
		if( i < stop - 1 ) {
			loff+=fprintf( stream, "," );
			switch( lineflag ) {
				case 0:
				if( loff >= 77) {
					lineflag=1;
					loff=0;
					fprintf( stream, "\\%s  ", stupid_cr );
				}
				break;
				case 1:
				if( loff >= 75 ) {
					loff=0;
					fprintf( stream, "\\%s  ", stupid_cr );
				}
				break;
			}
			//	if( 24 < i || 0 == (i - 17) % 25 ) fprintf( stream, "\\\n  " );
		}
	}

	//   fprintf( stream, "\n" );
} // }}}

//TODO: These are gross
char *thbuf = NULL;
int thlen = 0;
char *tohex( char *hbuf, int start, int stop ) // {{{ Dumps plain hex into a static buffer and returns it
{
	int i = start;

	int nblen = (stop - start) * 3 + 1;
	if( nblen > thlen ) {
		thbuf = F_REALLOC( thbuf, nblen );
		thlen = nblen;
	}

	for( i=0; i<stop - start; i++ ) {
		sprintf( thbuf + i*3, "%02X ", (unsigned char)*(hbuf + start +i) );
	}

	thbuf[ (stop - start)*3 ] = '\0';

	return thbuf;

} // }}}
void hexdump(char *hbuf, int start, int stop, int ascii) /* {{{ HexDump all or a part of some buffer */
{
	char c;
	int diff,i;

	while (start < stop ) {
		diff = stop - start;
		if (diff > 16) diff = 16;

		fprintf(stderr, ":%08X  ",start);

		for (i = 0; i < diff; i++) {
			if( 8 == i ) fprintf( stderr, " " );
			fprintf(stderr, "%02X ",(unsigned char)*(hbuf+start+i));
		}
		if (ascii) {
			for (i = diff; i < 16; i++) fprintf(stderr, "   ");
			for (i = 0; i < diff; i++) {
				c = *(hbuf+start+i);
				fprintf(stderr, "%c", isprint(c) ? c : '.');
			}
		}
		fprintf(stderr, "\n");
		start += 16;
	}
}
// }}}
// }}}

// {{{ Ugly File Reader - reads utf8/utf16 with dos/unix EOL characters
struct uglyread *ugly_open( char *path ) // {{{ Horrible, just horrible
{
	struct uglyread *uh = NULL;

	uh = F_MALLOC( sizeof( struct uglyread ) );

	if( NULL == ( uh->f = fopen( path, "r" ) ) ) {
		ERR_UNIX( errno, "%s", path );
		free( uh );
		return NULL;
	}

	uh->type = 0;
	uh->lines = 0;
	uh->vb = vbuf_alloc( 10 );
	uh->vb->dlen = 0;

	return uh;

} // }}}
void ugly_close( struct uglyread *th ) // {{{
{
	fclose( th->f );
	vbuf_free( th->vb );
	free( th );
} // }}}
int ugly_eof( struct uglyread *uh ) // {{{ file is eof and our buffer is empty
{
	if( feof( uh->f ) && uh->vb->dlen == 0 ) return 1;
	return 0;
} // }}}
int skip_nl( char *s ) // {{{ returns the width of the newline at s[0]
{
	if( s[0] == '\n' ) return 1;
	if( s[0] == '\r' && s[1] == '\n' ) return 2;
	if( s[0] == '\0' ) return 0;
	return -1;
} // }}}
int find_nl( struct varbuf *vb ) // {{{ find newline of type type in b
{
	char *nextr, *nextn;

	nextr = memchr( vb->b, '\r', vb->dlen );
	nextn = memchr( vb->b, '\n', vb->dlen );

	//case 1: UNIX, we find \n first
	if( nextn && (nextr == NULL || nextr > nextn ) ) {
		return nextn - vb->b;
	}

	//case 2: DOS, we find \r\n
	if( NULL != nextr && NULL != nextn && 1 == (char*)nextn - (char*)nextr ) {
		return nextr - vb->b;
	}

	//case 3: we find nothing

	return -1;
} // }}}
// }}}
// {{{ Parsing/Tokenizing things
int debugit(char *buf, int sz) /* {{{ Simple buffer debugger, returns 1 if buffer dirty/edited */
{
	char inbuf[100],whatbuf[100],*bp;

	int dirty=0,to,from,l,i,j,wlen,cofs = 0;

	printf("Buffer debugger. '?' for help.\n");

	while (1) {
		l = fmyinput(".",inbuf,90);
		bp = inbuf;

		skipspace(&bp);

		if (l > 0 && *bp) {
			switch(*bp) {
				case 'd' :
					bp++;
					if (*bp) {
						from = gethex(&bp);
						to   = gethex(&bp);
					} else {
						from = cofs; to = 0;
					}
					if (to == 0) to = from + 0x100;
					if (to > sz) to = sz;
					hexdump(buf,from,to,1);
					cofs = to;
					break;
				case 'a' :
					bp++;
					if (*bp) {
						from = gethex(&bp);
						to   = gethex(&bp);
					} else {
						from = cofs; to = 0;
					}
					if (to == 0) to = from + 0x100;
					if (to > sz) to = sz;
					hexdump(buf,from,to,0);
					cofs = to;
					break;
#if 0
				case 'k' :
				bp++;
				if (*bp) {
				from = gethex(&bp);
				} else {
				from = cofs;
				}
				if (to > sz) to = sz;
				parse_block(from,1);
				cofs = to;
				break;
#endif
#if 0
				case 'l' :
				bp++;
				if (*bp) {
				from = gethex(&bp);
				} else {
				from = cofs;
				}
				if (to > sz) to = sz;
				nk_ls(from+4,0);
				cofs = to;
				break;
#endif
				case 'q':
					return(0);
					break;
				case 's':
					if (!dirty) fprintf(stderr, "Buffer has not changed, no need to write..\n");
					return(dirty);
					break;
					case 'h':
					bp++;
					if (*bp == 'a') {
						from = 0;
						to = sz;
						bp++;
					} else {
						from = gethex(&bp);
						to   = gethex(&bp);
					}
					wlen = gethexorstr(&bp,whatbuf);
					if (to > sz) to = sz;
					printf("from: %x, to: %x, wlen: %d\n",from,to,wlen);
					for (i = from; i < to; i++) {
						for (j = 0; j < wlen; j++) {
							if ( *(buf+i+j) != *(whatbuf+j)) break;
						}
						if (j == wlen) printf("%06x ",i);
					}
					printf("\n");
					break;
				case ':':
					bp++;
					if (!*bp) break;
					from = gethex(&bp);
					wlen = gethexorstr(&bp,whatbuf);

					printf("from: %x, wlen: %d\n",from,wlen);

					memcpy(buf+from,whatbuf,wlen);
					dirty = 1;
					break;
#if 0
				case 'p':
				j = 0;
				if (*(++bp) != 0) {
				from = gethex(&bp);
				}
				if (*(++bp) != 0) {
				j = gethex(&bp);
				}
				printf("from: %x, rid: %x\n",from,j);
				seek_n_destroy(from,j,500,0);
				break;
#endif
				case '?':
					printf("d [<from>] [<to>] - dump buffer within range\n");
					printf("a [<from>] [<to>] - same as d, but without ascii-part (for cut'n'paste)\n");
					printf(": <offset> <hexbyte> [<hexbyte> ...] - change bytes\n");
					printf("h <from> <to> <hexbyte> [<hexbyte> ...] - hunt (search) for bytes\n");
					printf("ha <hexbyte> [<hexbyte] - Hunt all (whole buffer)\n");
					printf("s - save & quit\n");
					printf("q - quit (no save)\n");
					printf("  instead of <hexbyte> etc. you may give 'string to enter/search a string\n");
					break;
				default:
					printf("?\n");
				break;
			}
		}
	}
}
// }}}

int vb_path_token( vbuf *tok, char **path ) // {{{ Path tokenizer: increments path, dumps token in tok, return TOK_$TYPE
{
	char *p;
	int is_esc, c;
	int result;

	result = TOK_EMPTY;
	vbuf_strset( tok, "" );

	p = *path;
	while( p && ( p = tok_esc_char( p, &is_esc, &c ) ) ) {
		if( is_esc ) {
			if( '\0' == c ) return TOK_ERROR;
			else continue;
		}

		if( result == TOK_EMPTY && '/' == c ) {
			vbuf_strset( tok, "/" );
			result = TOK_DELIM;
			while('/' == *p ) p++;
			break;
		}

		if( '/' == c || '\0' == c ) {
			if( p > *path ) {
				if( '/' == c )p--;
				vbuf_strnset( tok, *path, p - *path );
				result = TOK_ELEMENT;
				if( 0 == strcmp( "..", tok->b ) ) result = TOK_PARENT;
				if( 0 == strcmp( ".", tok->b ) ) result = TOK_CURRENT;
			} else {
				result = TOK_EMPTY;
			}

			break;
		}
		result = TOK_ELEMENT;
	}

	*path = p;
	return result;

	DIE("This should *probably* never execute");
	return TOK_ERROR;
}
// }}}
int gettoken( char *tok, int len, char **path, char delim ) // {{{ Path tokenizer: increments path, dumps token in tok, return TOK_$TYPE
{
	// in: tok - buffer for next token, len - length of tok, **path - current position in path; out: tok, path; returns: TOK_(TYPE)
	char *next;

	if( len < 2 ) return TOK_BUF_SMALL;

	if( *path == NULL || **path == '\0' ) {
		*tok = '\0';
		return TOK_EMPTY;
	}

	if( **path == delim ) { /* leading delimiters imply absolute paths */
		tok[0] = delim;
		tok[1] = '\0';

		while( **path == delim )
			(*path)++;
		
		return TOK_DELIM;
	}

	while(1){
		next = strchr( *path, delim );
		if( !next ) {
			next = *path + strlen( *path );
			ASSERT( next[0] == '\0', "Gak!" );
		}

		if( next - *path >= len )
			return TOK_BUF_SMALL;

		memcpy( tok, *path, next - *path );
		tok[ next - *path ] = '\0';

		*path = next;

		//skip trailing delimiters
		while( **path == delim ) (*path)++;

		if( 0 == strcmp( "..", tok ) ) return TOK_PARENT;
		if( 0 == strcmp( ".", tok ) ) return TOK_CURRENT;

		return TOK_ELEMENT;
	}
}
// }}}
int is_delim( char c, char *delim ) // {{{
{
	int i=0;
	while( delim[i] ) {
		if( c == delim[i] ) return 1;
		i++;
	}

	return 0;
} // }}}
void normalize_path( char *p, char *delim) //{{{ in: "\\broken\\\\path/with////mixed/slashes"; out: "\\clean\\path\\"
{
        int i=0;
        int blen = strlen(p);
	char pugly[2];
	char pugfugly[4]; 
	char *p2;

	snprintf( pugly, 2, "%c", delim[0] );
	snprintf( pugfugly, 4, "%c.%c", delim[0], delim[0] );

	//WARN("normalize_path() in: %s", p );

        //change forward slashes to backslashes (because escape characters are great to puke everywhere)
        for( i=0; i<blen; i++ ) if( is_delim(p[i], delim) ) p[i] = delim[0];

        //eat multiple backslashes
        for( i=0; i<blen; i++ ) {
                if( is_delim( p[i], delim ) ){
                        int end=i;
                        while( end < blen && is_delim( p[end], delim ) ) end++;
                        if( end > i + 1) {
                                memmove( &p[i]+1, &p[end], blen-i );
                        }
                }
        }

        // /./ -> /
	while( NULL != ( p2 = strstr( p, pugfugly ) ) ) {
		memmove( p2, p2 + strlen( pugfugly ) - strlen( pugly ), strlen( p2 ) );
	}
        //WARN("normalize_path() out: %s", p );

} // }}}
char *tok_esc_char( char *s, int *is_esc, int *c ) // {{{ in: s = '\tstuff\t', out: c = real(\t), returns: &'stuff\t'
{
	if(is_esc) *is_esc = 0;
	if( '\\' == *s ) {
		if(is_esc) *is_esc = 1;
		s++;
		switch( *s ) {
			case '\\': c && (*c = '\\'); break;
			case 'a': c && (*c = '\a'); break;
			case '"': c && (*c = '"'); break;
			case '0': c && (*c = '0'); break;
			case '/': c && (*c = '/'); break;
			case 'b': c && (*c = '\b'); break;
			case 't': c && (*c = '\t'); break;
			case 'n': c && (*c = '\n'); break;
			case 'v': c && (*c = '\v'); break;
			case 'f': c && (*c = '\f'); break;
			case 'r': c && (*c = '\r'); break;
			default:
				//TODO: error
				ERR_UNIX( EILSEQ, "tok_esc_char(): unknown escape sequence(\\(%d:0x%x)): char:'%c' in %s.", *s, *s, *s, s);
				return NULL;
				break;
		}
		return ++s;
	}

	*c = *s;

	if(!*s) return s;
	return ++s;
} // }}}
char *esc_index( char *s, int c ) // {{{ just like index(3), but works on strings with escape sequences
{
	char *p = s;
	int is_esc;
	while( *p ) {
		int c2;
		char *p0 = p;
		is_esc = 0;
		p = tok_esc_char( p, &is_esc, &c2 );
		if( !p ) { 
			DIE("tok_esc_char() failed: %s", uerr_str( uerr_get() ) );
			//return NULL;
		}
		if( !is_esc && c == c2 ) {
			//fprintf(stderr, "esc_index( %p %s, %d ) -> %p %s\n", s, s, c, p0, p0 );
			return p0;
		}
	}

	return NULL;
} // }}}
char *esc_rindex( char *s, int c ) // {{{ just like rindex(3), but skips escaped characters
{
	char *lp=NULL;
	char *p=s;

	p = s;
	while( ( p = esc_index( p, c ) ) ) {
		lp = p;
		p++;
	}
	p=lp;
	return lp;
} // }}}
int parse_escaped_string( struct varbuf *vb, char *str, int len ) // {{{ in: 'blah\nblah\a' out: "blah" + real(\n) + "blah" + real(\a)
{
        int off=0;
        char *nbs;

	vbuf_strset( vb, "" );

        while( off < len ) {
                nbs = index( str + off, '\\' );
		if( nbs - ( str + off ) >= len ) nbs = NULL;
                if( NULL == nbs ) {
                        vbuf_strncat( vb, str + off, len - off );
                        return 0;
                }
		else {
			int skip = 2;
                        vbuf_strncat( vb, str + off, nbs - str - off );
			switch( (nbs + 1)[0] ) {
				case '\\':
					vbuf_strncat( vb, "\\", 1 );
					break;
				case 'a':
					vbuf_strncat( vb, "\a", 1 );
					break;
				case 'b':
					vbuf_strncat( vb, "\b", 1 );
					break;
				case 't':
					vbuf_strncat( vb, "\t", 1 );
					break;
				case 'n':
					vbuf_strncat( vb, "\n", 1 );
					break;
				case 'v':
					vbuf_strncat( vb, "\v", 1 );
					break;

				case 'f':
					vbuf_strncat( vb, "\f", 1 );
					break;

				case 'r':
					vbuf_strncat( vb, "\r", 1 );
					break;

				default:
					skip = 1;
					//vbuf_strncat( vb, "\\", 1 );
					break;

			}

			off = nbs - str + skip;
		}
        }

	return 0;
} // }}}
// }}}

char *str_dup( const char *str ) // {{{ duplicate string
{
	char *str_new;

	if (!str)
	return 0 ;

	//CREATE( str_new, char, strlen(str) + 1 );
	str_new = F_MALLOC( strlen(str) + 1 );
	strcpy( str_new, str );
	return str_new;
}
// }}}
int find_in_buf(char *buf, char *what, int sz, int len, int start) /* {{{ general search routine, find something in something else */
{
	int i;

	for (; start < sz; start++) {
		for (i = 0; i < len; i++) 
			if (*(buf+start+i) != *(what+i)) break;
		if (i == len) return(start);
	}
	return(0);
}
// }}}
int get_int( char *array ) /* {{{ Get INTEGER from memory. This is probably low-endian specific? */
{
	return ((array[0]&0xff) + ((array[1]<<8)&0xff00) +
		   ((array[2]<<16)&0xff0000) +
		   ((array[3]<<24)&0xff000000));
}
// }}}
void skipspace(char **c) /* {{{ increments *c until it isn't pointing at a ' ' */
{
	while( **c == ' ' ) (*c)++;
}
// }}}
