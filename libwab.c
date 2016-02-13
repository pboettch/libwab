// {{{ Includes
#include "libwab.h"
#include "tools.h"
#include "uerr.h"
#include "cencode.h"

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <time.h>
#include <errno.h>

// }}}

#ifndef HAVE_ICONV
#warning "ICONV IS MISSING!  Unicode will be BADLY broken"
#endif
//TODO: the suffix stuff was for generating DOS EOLs.  It's ugly, conflicts
//with base64 encode (which appends a \n) and seems pointless
//#define ASSERT(x)   if( !(x) ){ fprintf( stderr, "Assertion failed.\n"); exit(1); }
#define MAX_OPCOUNT 500 

//set this to one and heuristic will try to load *anything*
//for testing only.
#define STUPID 0
#define OFFSET_20 (sizeof(int) * (5) )

//static int use_unicode = 0;

//int skip_crud=1;
int dodebug=1;
int rread_crash=1;
int temp_message_printed=0;

static int nopen = 0;

// {{{ Proto
size_t utf16_strlen( char *src ); 
void output_ldifline( vbuf *vb, char *id, char *in, char *suffix ); 
int lwutf16to8( vbuf *vb, char *src ); 
char *m_cheap_uni2ascii( char *src );
void print_subrec( FILE *fp, int opno, struct wab_record *wrec, char *prefix, char *suffix ); 
void output_subrecord( vbuf *vb, int opno, struct wab_record *wrec, char *prefix, char *suffix );
int read_opdatum( struct subrecref *srec, int opcode, void **p );
char *id_get_str2( int opcode ); 
char *ldid_get_str2( int opcode ); 
// }}}

//void enable_unicode() { use_unicode = 1; }
struct wab_handle *open_wab( char *filename ) // {{{
{
	struct wab_handle *wh;
	nopen++;

#ifdef HAVE_ICONV
	unicode_init();
#endif

	if( NULL == ( wh = (struct wab_handle*)malloc( sizeof( struct wab_handle ) ) ) ) {
		fprintf( stderr, "open_wab(): Error malloc()ing the wab_handle\n");
		exit(1);
	}

	memset( wh, 0, sizeof( struct wab_handle ) );

	if( NULL == ( wh->fp = fopen( filename, "rb" ) ) ) {
		fprintf( stderr, "open_wab(): Error opening file \"%s\".\n", filename );
		exit(1);
	}

	rread( &wh->wabhead, sizeof( struct wab_header ), wh->fp );

	return wh;
}
// }}}
int close_wab( struct wab_handle *wh ) // {{{
{
	fclose( wh->fp );
	free( wh );

	nopen--;

	if( 0 == nopen ) {
#ifdef HAVE_ICONV
		unicode_close();
#endif
	}
	return 0;
}
// }}}

// {{{ Read and decode
int read_opdatum( struct subrecref *srec, int opcode, void **p ) // {{{ IN: *p points to subrecord, opcode identifies type of subrecord, opdata is allocated; OUT: opdata is populated, *p points to the next subrecord
{
	//The opdata should start with the opcode (which we are given)
	//rread( &opbuf, sizeof( int ), fp );

	if( *(int*)*p != opcode ) {
		fprintf( stderr, "read_opdatum(): given opcode 0x%x != read opcode 0x%x\n",
		opcode, *(int*)*p);
		return -1;
	}

	*p += sizeof( int );

	//read in data
	switch( opcode & 0xffff ) {
		case MT_SINT16: //Signed 16bit value
		case MT_SINT32: //Signed 32bit value
		case MT_BOOL: //Boolean (non-zero = true)
		case MT_EMBEDDED: //Embedded Object
		case MT_STRING: //Null terminated String
		case MT_UNICODE: //Unicode string
		case MT_SYSTIME: //Systime - Filetime structure
		case MT_OLE_GRID: //OLE Guid
		case MT_BINARY: //Binary data
			//allocate our buffer
			srec->len = (int*)*p;
			*p += sizeof( int );
			srec->data = *p;
			*p += *srec->len;

			DEBUG( DB_VERBOSE2, fprintf( stderr, "Size of subrecord, (0x%x) == %d(0x%d)\n", opcode, *srec->len, *srec->len); );
			break;

		case MT_SINT32_ARRAY: // Array of 32bit values
			fprintf( stderr, "TODO: WARNING: MT_SINT32_ARRAY LOADING UNTESTED!!!\n");
			//TODO: don't die, just skip record
			fprintf( stderr, "TODO: don't die here.\n");
			exit(1);
			break;

		case MT_BINARY_ARRAY: // Array of Binary data
		case MT_STRING_ARRAY: // Array of Strings
		case MT_UNICODE_ARRAY: // Array of Unicode
		{
			//the format here is:
			//[int: element count (already read at this point)][int: arraydatasize (pending read)]
			// [int elementlength][element]
			// [int elementlength][element]
			// ...

			srec->acnt = (int*)*p;
			*p += sizeof( int );
			srec->len = (int*)*p;
			*p += sizeof( int );


			DEBUG( DB_VERBOSE2, fprintf( stderr, "Unicode array element count: %d (0x%x)\n", *srec->acnt, *srec->acnt ); )
			DEBUG( DB_VERBOSE2, fprintf( stderr, "Unicode array data length: %d (0x%x)\n", *srec->len, *srec->len ); )

			srec->data = *p;
			*p += *srec->len;
			break;
		}

		default:
		printf ("Unknown data packing format found 0x%x\n", opcode);
		return -1;
	}

	return 0;
}
// }}}
int read_txtrec( struct txtrecord *trec, FILE *fp ) // {{{
{
	char strbuf[STR_SIZE];
	int thisoff = ftell( fp );

	if( 0 != rread( strbuf, STR_SIZE, fp ) ) return -1;
	if( 0 != rread( &trec->recid, sizeof( int ), fp ) ) return -1;

	cheap_uni2ascii( strbuf, trec->str, STR_SIZE);

	fprintf( stderr, "  0x%x: Index %d(%x), String: %s\n",
		thisoff, trec->recid, trec->recid, trec->str );

	return 0;
}
// }}}
int read_idxrec( struct idxrecord *irec, FILE *fp ) // {{{
{
	int thisoff=ftell( fp );
	if( 0 != rread( irec, sizeof( struct idxrecord ), fp ) ) return -1;

	fprintf( stderr, "  0x%x: record id: %d(0x%x) offset: %x\n",
		thisoff, irec->recid, irec->recid, irec->offset );

	return 0;
}
// }}}
int rread( void *buf, size_t size, FILE *fp ) // {{{
{
	DEBUG( DB_LOW_LEVEL, fprintf( stderr, "rread() reading %zd(0x%zx) bytes at 0x%lx\n", size, size, ftell( fp ) ););

	size_t len;
	long off1 = ftell( fp );

	if( size != ( len = fread( buf, 1, size, fp ) ) ) {
		fprintf( stderr, "ERROR: [%zd:%d:%d], Couldn't read %zd bytes at %#lx, now at %#lx: %s\n",
			len, ferror( fp ), feof( fp ), size, off1, ftell( fp ), strerror( errno ) );

		if( rread_crash ) exit( 1 );
		else return -1;
	}

	return 0;
}
// }}}
void rfseek( int off, FILE *fp ) // {{{ seek or die
{
	if( 0 != fseek( fp, off, SEEK_SET ) ) {
		fprintf( stderr, "Error seeking to %x\n", off );
		exit(1);
	}
}
// }}}
int check_rhead( struct rec_header *rhead ) // {{{
{
	//some inconsistant consistancy checks
	if( rhead->mystery4 != 0x20 ) {
		fprintf( stderr, "THERE'S A RECORD IN HERE WITH MYSTERY4 != 20!!!\n");
		DEBUG(DB_DITCH, exit(1););
	}

	if( rhead->mystery5 != rhead->mystery6 ) {
		fprintf( stderr, "THERE'S A RECORD IN HERE WITH MYSTERY5 != MYSTERY6!!!\n");
		DEBUG(DB_DITCH, exit(1);)
	}

	if( rhead->opcount > MAX_OPCOUNT ) {
		fprintf(stderr, "ERROR: opcount (%d) is larger than MAX_OPCOUNT (%d).  Probable corruption.\n",
			rhead->opcount, MAX_OPCOUNT );
		return -1;
	}

	return 0;
} // }}}
int read_oplist( int **oplist, int count, FILE *fp ) // {{{ IN: *oplist == NULL or malloc()d, count == no elements, fp is at correct offset; OUT: oplist is populated with list of integers
{
	int r,i;
	// Now we load the opcode list // realloc it
	DEBUG(DB_VERBOSE2, fprintf(stderr, "Reading opcode list (%d)\n", count ));
	if( NULL == (*oplist = (int*)realloc( *oplist, count * sizeof( int ) ) ) ) {
		fprintf( stderr, "ERROR: read_wab_record(): can't realloc() oplist.\n");
		return -1;
	}

	// And read it
	if( 0 != ( r = rread( *oplist, sizeof( int ) * count, fp ) ) )
		return r;

	DEBUG(
		DB_DATA_DUMP,
		// now we dump it
		for( i=0; i<count; i++)
		fprintf( stderr, "OPLIST[%d]: %d(0x%x)\n", i, *oplist[i], *oplist[i]  );
	);

	return 0;

} // }}}
int read_rec_header( struct rec_header *rhead, FILE *fp ) // {{{ IN: rhead is allocated; OUT: rhead is populated
{
	int r;
	if( 0 != (r = rread( rhead, sizeof( struct rec_header ), fp ) ) )
		return r;

	if( 0 != ( r = check_rhead( rhead ) ) )
		return r;

	DEBUG(
		DB_DATA_DUMP,
		dump_rec_head( rhead )
	     );

	return 0;
} // }}}
int read_subrecords( struct wab_record *wrec, FILE *fp ) // {{{
{
	int r;
	void *p;
	int i;

	if( NULL == (wrec->srecs=
		(struct subrecref*)realloc( wrec->srecs, wrec->head.opcount * sizeof( struct subrecref ) ) ) ) {
		fprintf( stderr, "ERROR: read_wab_record(): can't realloc() opdata.\n");
		return -1;
	}

	if( NULL == (wrec->data=(void*)realloc( wrec->data, wrec->head.datalen ) ) ) {
		fprintf( stderr, "ERROR: couldn't allocate memory for record.\n");
		return -1;
	}

	memset( wrec->srecs, 0, wrec->head.opcount * sizeof( struct subrecref ) );

	if( 0 != ( r = rread( wrec->data, wrec->head.datalen, fp ) ) )
		return r;

	p = wrec->data;

	//populate opdata pointer table
	for(i=0; i<wrec->head.opcount; i++ ) {
		DEBUG( DB_VERBOSE2, fprintf( stderr, "Reading opdata %d\n", i ); );
		if( 0 != read_opdatum( &(wrec->srecs[i]), wrec->oplist[i], &p ) ) {
			fprintf( stderr, "Error reading opdata, can't continue in record. :(\n");
			return -1;
		}
	}

	return 0;
} // }}}
int read_wab_record( struct wab_record *wrec, FILE *fp ) // {{{ IN: fp is pointing at a wab record, wrec is allocated; OUT: 
//wrec->oplist MUST be malloced() or NULL
//realloc() is used here, you must free()
{
	//int i;
	//struct rec_header *rhead;
	//void *p;
	int r;
	//rhead = &wrec->rhead;

	if( 0 != (r = read_rec_header( &wrec->head, fp ) ) )
		return r;

	if( 0 != (r = read_oplist( &wrec->oplist, wrec->head.opcount, fp ) ) )
		return r;

	if( 0 != ( r = read_subrecords( wrec, fp ) ) )
		return r;


	DEBUG( DB_VERBOSE2, fprintf( stderr, "Record loaded successfully...\n"); );

	return 0;
}
// }}}
struct subrecref *wrec_getfield( struct wab_record *mrec, int opcode ) // {{{
{
//TODO: there's some UGLY pointer stuff going on with **p below...
//TODO: this function is UUUUUUUUUGLY
	int i;
	for(i=0; i<mrec->head.opcount; i++) {
		DEBUG(DB_VERBOSE2, fprintf(stderr, "wrec_getfield(): oplist %d/%d: 0x%x for 0x%x\n",
			i, mrec->head.opcount, mrec->oplist[i], opcode) );
		if( ((mrec->oplist[i] >> 16) & 0xffff) == opcode ) {
			DEBUG(DB_VERBOSE2, fprintf(stderr, "found it...\n"); );
			return &mrec->srecs[i];
		}
	}
	return NULL;
}
// }}}
char *wrec_getstring( struct wab_record *mrec, int opcode ) // {{{
{
	struct subrecref *srec = wrec_getfield( mrec, opcode );
	//TODO: check if this is really a string and return null if not (the whole
	//point of this function)
	if( NULL == srec )
		return NULL;
	return srec->data;
}
// }}}
int do_heuristic( char *path ) // {{{
{
	struct wab_handle *wh;
	struct wab_record wrec;
	struct rec_header rhead;
	int my20;

	rread_crash = 0;

	DEBUG(DB_VERBOSE, fprintf(stderr, "Entering heuristic mode.\n"));

	memset( &wrec, 0, sizeof( struct wab_record ) );

	if( NULL == ( wh = open_wab( path ) ) ) {
		fprintf( stderr, "Error opening %s\n", path );
		return 1;
	}

	if( 0 != fseek( wh->fp, 0, SEEK_SET ) ) {
		fprintf( stderr, "Error seeking to start of file. \n" );
		return 1;
	}

	DEBUG(DB_VERBOSE, fprintf(stderr, "Starting search...\n"));
	while( ! feof( wh->fp ) ) {
		if( 1 != fread( &my20, sizeof( my20 ), 1, wh->fp) ) {
			if( feof( wh->fp ) ) {
				fprintf( stderr, "Heuristic hit EOF.  All done.\n" );
				return 1;
			}
			else {
				fprintf( stderr, "Error reading from file.\n" );
				return 1;
			}
		}

		long lastoff = ftell( wh->fp );

		if( STUPID || 0x20 == my20 ) { //found *something*...try to load it

			DEBUG(DB_VERBOSE,
				fprintf(stderr, "Found a possible record at %lx -> %lx\n", lastoff, lastoff - OFFSET_20 ));

			//Assuming this is a header then we go to where the start would be
			if( 0 == fseek( wh->fp, - OFFSET_20, SEEK_CUR ) ) {
				DEBUG(DB_VERBOSE, fprintf(stderr, "Trying to read@%lx...\n", ftell( wh->fp ) ) );
				rread( &rhead, sizeof( struct rec_header ), wh->fp );
				//dump_rec_head( &rhead );
				if( STUPID || rhead.mystery0 == 0 || rhead.mystery0 == 1 ) {
					fseek( wh->fp, lastoff - OFFSET_20, SEEK_SET );

					//if( 0 == rhead.mystery0 ) { fprintf(stderr, "DELETED...\n"); }
					DEBUG(DB_VERBOSE, fprintf(stderr, "This one is probable: %lx\n", ftell( wh->fp ) ) );
					if( 0 != read_wab_record( &wrec, wh->fp ) ) {
						fprintf( stderr, "read_wab_record() couldn't load a record.." );
					}
					else {
						//fprintf(stderr, "Output...\n");
						if( 0 != write_ldif( stdout, &wrec ) ) {
							fprintf(stderr, "write_ldif() failed.\n");
						}
						//fprintf(stderr, "...done\n");
					}
				}
			}
		}

		//we seek to just after the 0x20 (even if an attempt to load
		//the last record was made it could have been broken)
		fseek( wh->fp, lastoff, SEEK_SET );
	}

	return 0;
}
// }}}
// }}}
// {{{ Generate output
int is_safe_init( unsigned char c ) // {{{ in: char; returns: 1 == rfc2849 SAFE-INIT-CHAR, 0 == not
{

	if(
			( 0x01 <= c && c <= 0x09 ) ||
			( 0x0b <= c && c <= 0x0c ) ||
			( 0x0e <= c && c <= 0x1f ) ||
			( 0x21 <= c && c <= 0x39 ) ||
			( 0x3b == c ) ||
			( 0x3d <= c && c <= 0x7f ) 
	)
		return 1;
	else
		return 0;
} // }}}
int is_safe_char( unsigned char c ) // {{{ in: char; returns: 1 == rfc2849 SAFE-CHAR, 0 == not
{
	if( 
		( 0x01 <= c && c <= 0x09 ) ||
		( 0x0b <= c && c <= 0x0c ) ||
		( 0x0e <= c && c <= 0x7f )
	)
		return 1;
	else
		return 0;
} // }}}
int is_safe( char *str ) // {{{ in: zero-terminated string; returns: 1 == rfc2849 SAFE-STRING, 0 == not
{
	char *p = str;

	//return 0;
	if( !is_safe_init( *p ) ) {
		//printf("EVIL CHAR: .%c.\n", *p );
		return 0;
	}
	p++;
	while( *p ) {
		if( !is_safe_char( *p ) ) {
			//printf("EVIL CHAR: .%c.\n", *p );
			return 0;
		}
		p++;
	}

	return 1;

} // }}}
void vbbase64( vbuf *vb, char *in ) // {{{ IN: in==zero terminated string; OUT: vb == base64(in) (RFC2045, 6.8)
{
	// This function modified from:
	//   b64enc.c - c source to a base64 encoder
	//
	//   This is part of the libb64 project, and has been placed in the public domain.
	//   For details, see http://sourceforge.net/projects/libb64

	const int readsize = 4096;
	char *plaintext = in;
	int len = strlen( in );
	int remain = len;
	char* code;
	int plainlength;
	int codelength;
	base64_encodestate state;

	code = (char*)malloc(sizeof(char)*readsize*2);
	
	base64_init_encodestate(&state);
	
	do {
		plainlength = (remain < readsize)? remain: readsize;
		codelength = base64_encode_block(plaintext, plainlength, code, &state);
		remain -= plainlength;
		vbuf_strncat( vb, code, codelength );
	}
	while ( remain > 0 );
	
	codelength = base64_encode_blockend(code, &state);
	vbuf_strncat( vb, code, codelength );
	
	free(code);
} // }}}
int write_ldif( FILE *dest, struct wab_record *mrec ) // {{{
{
	VBUF_STATIC( tmp, 10 );
	VBUF_STATIC( tmp2, 10 );
	VBUF_STATIC( vbdn, 10 );
	VBUF_STATIC( opbuf, 10 );
	VBUF_STATIC( usdec, 10 );
	VBUF_STATIC( vb, 10 );
	//char *s, *us;
	char *us;
	//char ldidstr[1024]; 
	int i;
	DEBUG( DB_VERBOSE2, fprintf( stderr, "write_ldif() is calling wrec_getstring()\n") );
	if( NULL == ( us = wrec_getstring( mrec, PR_DISPLAY_NAME ) ) ) {
		fprintf( stderr, "Couldn't read display name for record.\n");
		return -1;
	}

	//hexdump( us, 0, utf16_strlen( us ) + 2, 1 );

	vbuf_strset( usdec, "" );
	if( 0 != lwutf16to8( usdec, us ) ) {
		fprintf(stderr, "iconv error: %s\n", strerror( errno ) );
		return -1;
	}
	//s = m_cheap_uni2ascii( us );

	if( 0 == strcmp( "Main Identity's Contacts", usdec->b ) ) return 0; //these seem to be junk records

	DEBUG( DB_VERBOSE2, fprintf( stderr, "write_ldif() is returning from wrec_getstring() %s\n", usdec->b) );

	if( is_safe( usdec->b ) ) 
		fprintf( dest, "# %s\n", usdec->b );
	else {
		fprintf( dest, "# \n" );
		//fprintf( dest, "# Can not express record as RFC2849 SAFE-STRING\n"  );
	}

	if( mrec->head.mystery0 == 0 ) {
		fprintf( dest, "# DELETED" );
	}

	vbuf_printf( vbdn, "cn=%s", usdec->b );

	vbuf_strset( vb, "" );
	output_ldifline( vb, "dn", vbdn->b, "\n" );
	output_ldifline( vb, "cn", usdec->b, "\n" );

	fprintf(dest, "%s", vb->b );
	/*
	vbuf_strset( tmp, "" );
	if( is_safe( usdec->b ) ) {
		fprintf( dest, "dn: %s\n", vbdn->b );
		vbuf_strset( tmp, "" );
		output_ldifstr( tmp, usdec->b, "\n" );
		fprintf( dest, "cn:%s", tmp->b );
	}
	else {
		vbuf_strset( tmp, "" );
		output_ldifstr( tmp, vbdn->b, "\n" );
		fprintf( dest, "dn:: %s", tmp->b );

		vbuf_strset( tmp, "" );
		output_ldifstr( tmp, usdec->b, "\n" );
		fprintf( dest, "cn:: %s", tmp->b );
	}
	*/

	//free( s );

	for( i=0; i<mrec->head.opcount; i++ ) {
		//VBUF_STATIC( base64buf, 10 );
		char *ldid;

		if( ((mrec->oplist[i] >> 16) & 0xffff) == PR_DISPLAY_NAME)
		continue;

		if( NULL == (ldid = ldid_get_str( (mrec->oplist[i] >> 16) & 0xffff ) ) ) {
			DEBUG(DB_VERBOSE2, fprintf(stderr, "Couldn't find ldid for 0x%x\n", (mrec->oplist[i] >> 16) & 0xffff); );
			continue;
		}

		//snprintf( ldidstr, 1024, "%s:", ldid );
		vbuf_strset( opbuf, "" );
		output_subrecord( opbuf, i, mrec, ldid, "\n" );
		printf( "%s", opbuf->b );
		//print_opdata( dest, i, mrec, ldidstr, "\n" );
	}


	fprintf( dest, "\n" );

	return 0;
}
// }}}
void binary_print( FILE *fp, unsigned char *buf, int len ) // {{{
{
	int i;
	//fprintf( stderr, "BIN[0x%x]:[", len);
	for( i=0; i<len; i++) {
		fprintf( fp, "%02x", (unsigned int)buf[i]);
		if( i<len-1 )fprintf( fp, " ");
	}
	if( sizeof( int ) < len ) {
		fprintf( fp, " " );
		for( i=0; i<len; i++) {
			if (buf[i] >= ' ' && buf[i] < 127) {
				fprintf( fp, "%c", buf[i] );
			} else {
				fprintf( fp, "." );
			}
		}
	}
	//fprintf( stderr, "]");
	if( sizeof( int ) == len ) {
		fprintf( stderr, " (int? %d) (unsigned int? %d)", *(int*)buf, *(unsigned int*)buf );
	}
	//fprintf( stderr, "\n");
} // }}}
void output_binary( vbuf *vb, unsigned char *buf, int len ) // {{{ in: vb==valid str; out: vb += (binary(buf,len))
{
	int i;
	//fprintf( stderr, "BIN[0x%x]:[", len);
	for( i=0; i<len; i++) {
		vbuf_printfa( vb, "%02x", (unsigned int)buf[i]);
		if( i<len-1 )vbuf_printfa( vb, " ");
	}
	if( sizeof( int ) < len ) {
		vbuf_printfa( vb, " " );
		for( i=0; i<len; i++) {
			if (buf[i] >= ' ' && buf[i] < 127) {
				vbuf_printfa( vb, "%c", buf[i] );
			} else {
				vbuf_printfa( vb, "." );
			}
		}
	}
	//fprintf( stderr, "]");
	if( sizeof( int ) == len ) {
		fprintf( stderr, " (int? %d) (unsigned int? %d)", *(int*)buf, *(unsigned int*)buf );
	}
	//fprintf( stderr, "\n");
} // }}}

// Unicode Bliss
void output_ldifline( vbuf *vb, char *id, char *in, char *suffix ) // {{{ IN: in == zero-terminated UTF8; OUT: vb .= (SAFE-STRING || : base64 )
{
	VBUF_STATIC( tmp, 100 );
	vbuf_strcat(vb, id);
	if( is_safe( in ) ) {
		vbuf_strcat( vb, ": ");
		vbuf_strcat( vb, in );
		vbuf_strcat( vb, suffix );
	}
	else {
		vbuf_strcat( vb, ":: ");
		vbuf_strset( tmp, "" );
		vbbase64( tmp, in );
		vbuf_strcat( vb, tmp->b );
	}
} // }}}
size_t utf16_strlen( char *src ) // {{{ returns no of bytes in *src
{
	size_t r=0;
	while( !( src[r] == '\0' && src[r+1] == '\0' ) ) {
		r+=2;
	}
	return r;
} // }}}
int lwutf16to8( vbuf *vb, char *src ) // {{{ in: src==utf16; out: vb==utf8 from src; returns: 0==success
{
	int r=0;
	vbuf_strset( vb, "" );
#ifdef HAVE_ICONV
	VBUF_STATIC( out, 100 );
	//hexdump( src, 0, utf16_strlen( src ) + 2, 1 );
	if( 0 != ( r = vb_utf16to8( out, src, utf16_strlen( src ) + 2 ) ) ) {
		fprintf(stderr, "Conversion from UTF16 to UTF8 failed: %s\n", uerr_str( uerr_get() ) );
		return r;
	}

	vbuf_strcat( vb, out->b );
#else
	char *p;
	for( p = src; !( p[0] == 0  && p[1] == 0 ); p+=2 ) {
		vbuf_charcat( vb, p[0] );
	}
#endif

	return r;
} // }}}
int output_unicode_str( vbuf *vb, char *buf ) // {{{ in: buf==utf16; out: vb+=(buf in utf8); returns: 0==success
{
	int r=0;
	VBUF_STATIC( tmp, 10 );
	if( 0 != ( r = lwutf16to8( tmp, buf ) ) ) {
		return r;
	}
	vbuf_strcat( vb, tmp->b );

	return r;
} // }}}
void print_unicode_str( FILE*fp, char *buf ) // {{{
{
	char *prbuf;
	prbuf = m_cheap_uni2ascii( buf );
	fprintf( fp, "%s", prbuf );
	free( prbuf );
}
// }}}
char *m_cheap_uni2ascii( char *src ) // {{{
{
#ifdef HAVE_ICONV
	VBUF_STATIC( out, 100 );
	if( 0 != vb_utf16to8( out, src, utf16_strlen( src ) + 2 ) ) {
		fprintf(stderr, "Conversion from UTF16 to UTF8 failed: %s\n", uerr_str( uerr_get() ) );
		return NULL;
	}

	return out->b;
#else
	int len=0;
	char *result = NULL;
	short *p= (short*)src;
	for( len=0; p[len] != 0; len++ );

	len = len*2;

	if( NULL == ( result = (char*)malloc( len + 2 ) ) )
		return NULL;

	cheap_uni2ascii( src, result, len );

	return result;
#endif
}
// }}}
void cheap_uni2ascii(char *src, char *dest, int l) // {{{
{
	for (; l > 0; l -=2) {
		*dest = *src;
		dest++; src +=2;
	}
	*dest = 0;
}
// }}}

void dump_subrecord( int opno, struct wab_record *wrec ) // {{{
{
	int opcode = wrec->oplist[opno];
	int id=(opcode>>16) & 0xffff;

	//spew data
	DEBUG( DB_DATA_DUMP, fprintf( stderr, "  Data[%x:%s:%s]: ", opcode, id_get_str2( id ), ldid_get_str2( id ) ); )

	//  DEBUG( DB_DATA_DUMP, fprintf( stderr, "%-40s ", id_get_str( id ) ); );
	switch( opcode & 0xffff )
	{

		case MT_SINT16:
		case MT_SINT32:
		case MT_BOOL:
		case MT_EMBEDDED:
		case MT_STRING:
		case MT_UNICODE:
			print_subrec( stderr, opno, wrec, "", "\n" );
			break;

		case MT_BINARY_ARRAY:
			print_subrec( stderr, opno, wrec, "BIN: ", "\n" );
			break;

		case MT_UNICODE_ARRAY:
		case MT_OLE_GRID:
		case MT_SYSTIME:
		case MT_BINARY:
			print_subrec( stderr, opno, wrec, "", "\n" );
			break;

		default:
			fprintf( stderr, "Unknown data type 0x%x\n", (opcode & 0xffff) );
		break;
	}

	return;
}
// }}}
void print_subrec( FILE *fp, int opno, struct wab_record *wrec, char *prefix, char *suffix ) // {{{
{
	int opcode = wrec->oplist[opno];
	//void *opdata = wrec->opdata[opno];
	struct subrecref *srec = &wrec->srecs[opno];
	int id=(opcode>>16) & 0xffff;
	//int *opbuf=NULL;

	//spew data
	//DEBUG( DB_VERBOSE2, fprintf( stderr, "  Data[%x:%s]: ", opcode, id_get_str2( id ) ); )

	//DEBUG( DB_VERBOSE, fprintf( stderr, "%-40s ", id_get_str2( id ) ); );
	switch( opcode & 0xffff ) {
		case MT_SINT16:
			fprintf( fp, "%s", prefix );
			fprintf( fp, "%hd", *((short int*)srec->data));
			fprintf( fp, "%s", suffix);
			break;

		case MT_SINT32:
			fprintf( fp, "%s", prefix );
			fprintf( fp, "%d", *((int*)srec->data));
			fprintf( fp, "%s", suffix);
			break;

		case MT_BOOL:
			fprintf( fp, "%s", prefix );
			{ //TODO: So a boolean is a single char?  Or an int?
				if( *(int*)srec->data )
					fprintf( fp, "TRUE");
				else
					fprintf( fp, "FALSE");
			}
			fprintf( fp, "%s", suffix);
			break;

		case MT_EMBEDDED:
			fprintf( fp, "%s", prefix );
			fprintf( stderr, "Can't print embedded object\n");
			fprintf( fp, "%s", suffix);
			break;

		case MT_STRING:
			fprintf( fp, "%s", prefix );
			fprintf( fp, "%s", (char*)srec->data );
			fprintf( fp, "%s", suffix);
			break;

		case MT_UNICODE:
			fprintf( fp, "%s", prefix);
			print_unicode_str( fp, srec->data );
			fprintf( fp, "%s", suffix);
			break;

		case MT_BINARY_ARRAY:
		{
			int size,i;
			int optype=((opcode >> 16) & 0xFFFF);
			void *p=srec->data;
			//memcpy( &elements, p, sizeof( int ) );
			//p+=sizeof( int );
			fprintf( stderr,
			"Looking at a binary array of %d elements (opcode 0x%x optype 0x%x opno 0x%x).\n",
			*srec->acnt, opcode, optype, opno );
			for( i=0; i<*srec->acnt; i++ ) {
				memcpy( &size, p, sizeof( int ));
				p+=sizeof( int );
				if ( optype == PR_MAB_MEMBER ) {
					char *buf;
					int j;
					fprintf( fp, "%s%s", prefix, "cn=" );
					buf = p;
					for ( j = 24; j < size; j += 2 ) {
						if ( buf[j] == '\0' ) {
							if ( j + 10 < size &&
								buf[j+2] == 'S' &&
								buf[j+4] == 'M' &&
								buf[j+6] == 'T' &&
								buf[j+8] == 'P' )
							{
								fprintf( fp, "%s", ",mail=" );
								j += 10;
								} else {
								if ( j + 2 < size ) {
									fprintf( fp, " " );
								}
							}
						} else {
							fprintf( fp, "%c", buf[j] );
						}
					}
				} else {
					fprintf( fp, "%d: %s", size, prefix );
					binary_print( fp, p, size );
				}
				p+=size;
				fprintf( fp, "%s", suffix);
			}
		}

		break;


		case MT_UNICODE_ARRAY:
		{
			int size,i;
			void *p=srec->data;

			DEBUG( DB_VERBOSE2,  fprintf( stderr, "%-40s ", id_get_str2( id ) ); )
			//memcpy( &elements, p, sizeof( int ) );
			//p+=sizeof( int );
			DEBUG( DB_VERBOSE2, fprintf( stderr, "Looking at a unicode array of %d elements.\n", *srec->acnt); );
			for( i=0; i<*srec->acnt; i++ ) {
				fprintf( fp, "%s", prefix );
				memcpy( &size, p, sizeof( int ));
				p+=sizeof( int );
				DEBUG(DB_VERBOSE2, fprintf( stderr, " element size: %d (0x%x)\n", size, size ); );
				print_unicode_str( fp, p );
				p+=size;
				fprintf( fp, "%s", suffix);
			}
		break;
		}


		case MT_STRING_ARRAY:
		{
			int size,i;
			void *p=srec->data;

			if( !temp_message_printed) { //TODO: remove this crap and the global variable.
				temp_message_printed=1;
				fprintf(stderr,
					"WARNING: Your wab file contains regular string array data.  I have NEVER seen\n"
					"a file with this data type myself.  This will *attempt* to decode this data but\n"
					"I don't know what will happen.  PLEASE check the results and send me an email\n"
					"(sloaring at tec-man.com) letting me know how things went.  Thanks\n"
					);
			}

			DEBUG( DB_VERBOSE2,  fprintf( stderr, "%-40s ", id_get_str2( id ) ); )
			//memcpy( &elements, p, sizeof( int ) );
			//p+=sizeof( int );
			DEBUG( DB_VERBOSE2, fprintf( stderr, "Looking at a unicode array of %d elements.\n", *srec->acnt); );
			for( i=0; i<*srec->acnt; i++ ) {
				fprintf( fp, "%s", prefix );
				memcpy( &size, p, sizeof( int ));
				p+=sizeof( int );
				DEBUG(DB_VERBOSE2, fprintf( stderr, " element size: %d (0x%x)\n", size, size ); );
				fprintf( fp, "%s", (char*)p );
				p+=size;
				fprintf( fp, "%s", suffix);
			}
		break;
		}

		case MT_OLE_GRID:
			fprintf( fp, "%s", prefix );
			fprintf( stderr, "I can't print an OLE GRID (I don't even know what one is).\n");
			fprintf( fp, "%s", suffix);
			break;

		case MT_SYSTIME:
			fprintf( fp, "%s", prefix );
			//struct tm datime;
			binary_print( fp, srec->data, *srec->len );
			fprintf( fp, "%s", suffix);
			break;

		case MT_BINARY:
			fprintf( fp, "%s", prefix );
			binary_print( fp, srec->data, *srec->len );
			fprintf( fp, "%s", suffix);
			break;

		default:
			fprintf( stderr, "Unknown data type 0x%x\n", (opcode & 0xffff) );
			break;
	}

	return ;
}
// }}}
void output_srec_data( vbuf *vb, int type, void *p, size_t len, char *prefix, char *suffix ) // {{ output simple, single-element subrecord datum
{
	VBUF_STATIC( tmp, 100 );
	//vbuf_printfa( vb, "%s", prefix );

	switch( type ) {
		case MT_SINT16:
			vbuf_printfa( vb, "%hd", *((short int*)p));
			vbuf_printfa( vb, "%s", suffix);
			break;

		case MT_SINT32:
			vbuf_printfa( vb, "%d", *((int*)p));
			vbuf_printfa( vb, "%s", suffix);
			break;

		case MT_BOOL:
			{ //TODO: So a boolean is a single char?  Or an int?
				if( *(int*)p )
					vbuf_printfa( vb, "TRUE");
				else
					vbuf_printfa( vb, "FALSE");
			}
			vbuf_printfa( vb, "%s", suffix);
			break;

		case MT_EMBEDDED:
			fprintf( stderr, "Can't print embedded object\n");
			vbuf_printfa( vb, "%s", suffix);
			break;

		case MT_STRING:
			//vbuf_printfa( vb, "%s", (char*)p );
			/*
			if( is_safe( (char*)p ) ) 
				vbuf_printfa(vb, "%s", " " );
			else
				vbuf_printfa(vb, "%s", ": " );
				*/
			output_ldifline( vb, prefix, (char*)p, suffix );
			//vbuf_printfa( vb, "%s", suffix);
			break;

		case MT_UNICODE:
			/*
			if( is_safe( (char*)p ) ) 
				vbuf_printfa(vb, "%s", " " );
			else
				vbuf_printfa(vb, "%s", ": " );
				*/
			if( 0 != lwutf16to8( tmp, p ) ) {
				fprintf(stderr, "Error: utf16 to 8 failed: %s\n", strerror( errno ) );
			}
			else {
				output_ldifline( vb, prefix, tmp->b, suffix );
				//output_unicode_str( vb, p );
			}
			//vbuf_printfa( vb, "%s", suffix);
			break;
		case MT_SYSTIME:
			//struct tm datime;
			output_binary( vb, p, len );
			vbuf_printfa( vb, "%s", suffix);
			break;

		case MT_BINARY:
			output_binary( vb, p, len );
			vbuf_printfa( vb, "%s", suffix);
			break;
		default:
			WARN("Unknown type: %d", type );
	};
}
void output_subrecord( vbuf *vb, int opno, struct wab_record *wrec, char *prefix, char *suffix ) // {{{
{
	VBUF_STATIC( tmp, 100 );
	vbuf_strset( vb, "" );
	int opcode = wrec->oplist[opno];
	//void *opdata = wrec->opdata[opno];
	struct subrecref *srec = &wrec->srecs[opno];
	int id=(opcode>>16) & 0xffff;
	//int *opbuf=NULL;

	//spew data
	//DEBUG( DB_VERBOSE2, fprintf( stderr, "  Data[%x:%s]: ", opcode, id_get_str2( id ) ); )

	//DEBUG( DB_VERBOSE, fprintf( stderr, "%-40s ", id_get_str2( id ) ); );


	switch( opcode & 0xffff ) {
		case MT_SINT16:
		case MT_SINT32:
		case MT_BOOL:
		case MT_EMBEDDED:
		case MT_STRING:
		case MT_UNICODE:
		case MT_SYSTIME:
		case MT_BINARY:
			output_srec_data( vb, opcode & 0xffff, srec->data, *srec->len, prefix, suffix );
			break;

		case MT_UNICODE_ARRAY:
		{
			int size,i;
			void *p=srec->data;

			DEBUG( DB_VERBOSE2,  fprintf( stderr, "%-40s ", id_get_str2( id ) ); )
			//memcpy( &elements, p, sizeof( int ) );
			//p+=sizeof( int );
			DEBUG( DB_VERBOSE2, fprintf( stderr, "Looking at a unicode array of %d elements.\n", *srec->acnt); );
			for( i=0; i<*srec->acnt; i++ ) {

				memcpy( &size, p, sizeof( int ));
				p+=sizeof( int );
				DEBUG(DB_VERBOSE2, fprintf( stderr, " element size: %d (0x%x)\n", size, size ); );
				//output_unicode_str( vb, p );
				output_srec_data( vb, MT_UNICODE, p, 0, prefix, suffix );
				/* {{{ old
				vbuf_printfa( vb, "%s", prefix );
				if( is_safe( (char*)p ) ) 
					vbuf_printfa(vb, "%s", " " );
				else
					vbuf_printfa(vb, "%s", ": " );
				if( 0 != lwutf16to8( tmp, p ) ) {
					fprintf(stderr, "Error: utf16 to 8 failed: %s\n", strerror( errno ) );
				}
				else {
					output_ldifstr( vb, tmp->b, suffix );
					//output_unicode_str( vb, srec->data );
				}

				//vbuf_printfa( vb, "%s", suffix);
				}}}*/
				p+=size;

			}
			break;
		}


		case MT_BINARY_ARRAY:
		{
			int size,i;
			int optype=((opcode >> 16) & 0xFFFF);
			void *p=srec->data;
			//memcpy( &elements, p, sizeof( int ) );
			//p+=sizeof( int );
			fprintf( stderr,
			"Looking at a binary array of %d elements (opcode 0x%x optype 0x%x opno 0x%x).\n",
			*srec->acnt, opcode, optype, opno );
			for( i=0; i<*srec->acnt; i++ ) {
				memcpy( &size, p, sizeof( int ));
				p+=sizeof( int );
				if ( optype == PR_MAB_MEMBER ) {
					char *buf;
					int j;
					vbuf_printfa( vb, "%s%s", prefix, "cn=" );
					buf = p;
					for ( j = 24; j < size; j += 2 ) {
						if ( buf[j] == '\0' ) {
							if ( j + 10 < size &&
								buf[j+2] == 'S' &&
								buf[j+4] == 'M' &&
								buf[j+6] == 'T' &&
								buf[j+8] == 'P' )
							{
								vbuf_printfa( vb, "%s", ",mail=" );
								j += 10;
								} else {
								if ( j + 2 < size ) {
									vbuf_printfa( vb, " " );
								}
							}
						} else {
							vbuf_printfa( vb, "%c", buf[j] );
						}
					}
				} else {
					vbuf_printfa( vb, "%d: %s", size, prefix );
					output_binary( vb, p, size );
				}
				p+=size;
				vbuf_printfa( vb, "%s", suffix);
			}
		}

		break;



		case MT_STRING_ARRAY:
		{
			int size,i;
			void *p=srec->data;

			if( !temp_message_printed) { //TODO: remove this crap and the global variable.
				temp_message_printed=1;
				fprintf(stderr,
					"WARNING: Your wab file contains regular string array data.  I have NEVER seen\n"
					"a file with this data type myself.  This will *attempt* to decode this data but\n"
					"I don't know what will happen.  PLEASE check the results and send me an email\n"
					"(sloaring at tec-man.com) letting me know how things went.  Thanks\n"
					);
			}

			DEBUG( DB_VERBOSE2,  fprintf( stderr, "%-40s ", id_get_str2( id ) ); )
			//memcpy( &elements, p, sizeof( int ) );
			//p+=sizeof( int );
			DEBUG( DB_VERBOSE2, fprintf( stderr, "Looking at a unicode array of %d elements.\n", *srec->acnt); );
			for( i=0; i<*srec->acnt; i++ ) {
				vbuf_printfa( vb, "%s", prefix );
				memcpy( &size, p, sizeof( int ));
				p+=sizeof( int );
				DEBUG(DB_VERBOSE2, fprintf( stderr, " element size: %d (0x%x)\n", size, size ); );
				vbuf_printfa( vb, "%s", (char*)p );
				p+=size;
				vbuf_printfa( vb, "%s", suffix);
			}
		break;
		}

		case MT_OLE_GRID:
			vbuf_printfa( vb, "%s", prefix );
			fprintf( stderr, "I can't print an OLE GRID (I don't even know what one is).\n");
			vbuf_printfa( vb, "%s", suffix);
			break;


		default:
			fprintf( stderr, "Unknown data type 0x%x\n", (opcode & 0xffff) );
			break;
	}

	return ;
}
// }}}
void dump_wab_header( struct wab_handle *wh ) // {{{
{
	int i;
	struct wab_header *whead = &wh->wabhead;

	fprintf( stderr, "Count 1: %d (0x%x)\n", whead->count1, whead->count1 );
	fprintf( stderr, "Count 2: %d (0x%x)\n", whead->count2, whead->count2 );

	for( i=0; i<TABLE_COUNT; i++ ) {
		fprintf( stderr, "Table %d\n", i );
		fprintf( stderr, "  type: %d (0x%x)\n", whead->tables[i].type, whead->tables[i].type );
		fprintf( stderr, "  size: %d (0x%x)\n", whead->tables[i].size, whead->tables[i].size );
		fprintf( stderr, "  offset: %d (0x%x)\n", whead->tables[i].offset, whead->tables[i].offset );
		fprintf( stderr, "  count: %d (0x%x)\n", whead->tables[i].count, whead->tables[i].count );
	}
}
// }}}
void dump_table( int table_id, struct wab_handle *wh ) // {{{
{
	//int recno=0; //store the record number
	//char rrec[STR_SIZE], rec[STR_SIZE]; //store unicode string and ascii string
	//int idxno=0;
	//int thisoff=0;
	int i;

	struct txtrecord trec;
	struct idxrecord irec;

	ASSERT( table_id < TABLE_COUNT, "table_id >= TABLE_COUNT" );

	struct tabledesc *tdesc = &wh->wabhead.tables[table_id];

	//seek to the start of the table
	if( -1 == fseek( wh->fp, tdesc->offset, SEEK_SET ) ) {
		fprintf( stderr, "Couldn't seek to table at offset %x\n.", tdesc->offset);
		exit(1);
	}

	//dump the table entries
	for( i = 0; i < tdesc->count; i++ ) {
		switch( tdesc->type ) {
			case TYPE_IDX:
				read_idxrec( &irec, wh->fp );
				break;

			case TYPE_TXT:
				read_txtrec( &trec, wh->fp );
				break;

			default:
				fprintf( stderr, "Unknown table type: %d (0x%x)\n", tdesc->type, tdesc->type);
				break;
		}
	}

}
// }}}
void dump_ldif_header( FILE *fp )  // {{{ output ldif header
{
	fprintf(fp, "version: 1\n" );
} // }}}
void output_records( struct wab_handle *wh ) // {{{ output records from wh to stdout in ldif format
{
	FILE *fp = wh->fp;
	int i,j;
	struct idxrecord irec;
	//struct rec_header mrec;
	int bookmark;
	int last=0;
	struct wab_record wrec;
	memset( &wrec, 0, sizeof( struct wab_record ) );

	for( i=0; i<TABLE_COUNT; i++ ) {
		if( wh->wabhead.tables[i].type == TYPE_IDX ) {
			rfseek( wh->wabhead.tables[i].offset, fp );

			for( j=0; j<wh->wabhead.tables[i].count; j++ ) {
				rread( &irec, sizeof( struct idxrecord ), fp );
				bookmark = ftell( fp );

				rfseek( irec.offset, fp );

				//TODO: read

				if( dodebug & DB_VERBOSE2 )
					fprintf( stderr, "\n*** Reading WAB entry %d of %d\n", j, wh->wabhead.tables[i].count);
				else if( dodebug & DB_VERBOSE2 )
					fprintf( stderr, "\n*** Reading WAB entry (%d/%d), id=0x%x at %x (from last: %x %x)\n",
					j, wh->wabhead.tables[i].count,
					irec.recid, irec.offset, irec.offset -
					last, (irec.offset - last)/2 );

				last = irec.offset;
				if( 0 != read_wab_record( &wrec, fp ) ) {
					fprintf(stderr, "Error reading WAB record.  Either the file is corrupt or libwab is broken.\n");

					fprintf(stderr, "If you think that libwab is screwing up then the author would just love a copy of your .wab file.\n");
				}
				DEBUG( DB_DATA_DUMP, dump_wab_record( &wrec ) );
				DEBUG( DB_LDIF_OUT, write_ldif( stdout, &wrec ); );


				rfseek( bookmark, fp );
			}
		}
	}
}
// }}}
void dump_wab_record( struct wab_record* mrec ) // {{{
{
	int i;
	for( i=0; i<mrec->head.opcount; i++) {
		DEBUG(DB_VERBOSE2, fprintf( stderr, "Dumping %d, %x\n", i, mrec->oplist[i]) );
		dump_subrecord( i, mrec );
	}
}
// }}}
void dump_rec_head( struct rec_header *rhead ) // {{{
{
	fprintf( stderr, " Mystery 0: %d(0x%x)\n", rhead->mystery0, rhead->mystery0 );
	fprintf( stderr, " Mystery 1: %d(0x%x)\n", rhead->mystery1, rhead->mystery1 );
	fprintf( stderr, " Index No: %d(0x%x)\n", rhead->recid, rhead->recid );
	fprintf( stderr, " Opcount: %d(0x%x)\n", rhead->opcount, rhead->opcount );
	fprintf( stderr, " Mystery 4: %d(0x%x)\n", rhead->mystery4, rhead->mystery4 );
	fprintf( stderr, " Mystery 5: %d(0x%x)\n", rhead->mystery5, rhead->mystery5 );
	fprintf( stderr, " Mystery 6: %d(0x%x)\n", rhead->mystery6, rhead->mystery6 );
	fprintf( stderr, " Data length: %d(0x%x)\n", rhead->datalen, rhead->datalen );
}
// }}}
char *id_get_str2( int opcode ) // {{{
{
	static char buf[ 80 ];
	char *result = id_get_str( opcode );
	if( NULL == result ) {
		sprintf( buf, "%s (0x%x)", "UNKNOWN OPCODE", opcode);
		result = buf;
	}

	return result;
}
// }}}
char *ldid_get_str2( int opcode ) // {{{
{
	static char buf[ 80 ];
	char *result = ldid_get_str( opcode );
	if( NULL == result ) {
		sprintf( buf, "%s (0x%x)", "UNKNOWN FDIF ATTRIB", opcode );
		result = buf;
	}

	return result;
}
// }}}
// }}}
// {{{ Dumpster
/*
char *get_ldid( int opcode ) // {{{
{
  switch( (opcode >> 16) & 0xffff )
  {
    case PR_DISPLAY_NAME:
    	return "dc";
    case PR_MAB_ADDRESS_STR:
    case PR_MAB_ALTERNATE_EMAILS:
	return "mail";

    default:
      return NULL;
  }
}
// }}}
*/
// }}}
