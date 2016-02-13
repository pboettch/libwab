#ifndef TOOLS_H
#define TOOLS_H
#define SZ_MAX     4096
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
/***************************************************/

// {{{ Tokenizer const TOK_EMPTY, TOK_ELEMENT, DELIM
#define DELIM '\\'

#define TOK_EMPTY	0
#define TOK_DELIM	1
#define TOK_PARENT	2
#define TOK_CURRENT	3
#define TOK_ELEMENT	4

#define TOK_ERROR	10
#define TOK_BUF_SMALL	11
// }}}

#define LOAD_DEBUG 1

#define DIE(...) { fprintf(stderr, "Fatal Error at %s,%d: ", __FILE__, __LINE__); pDIE(__VA_ARGS__); }


//#define WARN(...) { fprintf(stderr, "WARN: %s,%d: ", __FILE__, __LINE__); pWARN(__VA_ARGS__); }
void pDIE( char *fmt, ... );
//void pWARN( char *fmt, ... );

#define WARN(...) DB( DB_WARN, __VA_ARGS__ )
#define ASSERT(x,...) { if( !(x) ) DIE( __VA_ARGS__ ); }

void *F_MALLOC( size_t size );
void *F_REALLOC( void *p, size_t size );

char *tohex( char *hbuf, int start, int stop );
void hexdump(char *hbuf, int start, int stop, int ascii); 

/* Various nice macros */

// {{{ CREATE and ALLOC
#define CREATE(result, type, number)\
    { \
        if (!((result) = (type *) calloc ((number), sizeof(type)))) { \
            perror("malloc failure"); \
            abort() ; \
       } \
    }
#define ALLOC(result, size, number)\
    { \
        if (!((result) = (void *) calloc ((number), (size)))) { \
            perror("malloc failure"); \
            abort() ; \
       } \
    }
// }}}
#define FREE(p) { free(p); (p) = 0; }

#define DO_DEBUG 0
// #define DEBUG(x) if( DO_DEBUG ) { x; }
#define stupid_cr "\r\n"

#define DB_CRASH   0 // crashing
#define DB_ERR     1 // error
#define DB_WARN    2 // warning
#define DB_INFO    3 // normal, but significant, condition
#define DB_VERB	   4 // verbose information
#define DB_0       5 // debug-level message
#define DB_1       6 // debug-level message
#define DB_2       7 // debug-level message

extern int DEBUG_LEVEL;
extern void (*dbfunc)(char *file, int line, int level, char *fmt, ...);

#define DB(...) { dbfunc( __FILE__, __LINE__, __VA_ARGS__ ); }

int set_db_function( void (*func)( char *file, int line, int level, char *fmt, ...) );

struct uglyread {
	FILE *f;
	int lines;
	struct varbuf *vb;
	int type;
};


/* Get a string of HEX bytes (space separated),
 * or if first char is ' get an ASCII string instead.
 */
int gethexorstr(char **c, char *wb);

/* Variable-length buffers */
struct varbuf { // {{{
	size_t dlen; 	//length of data stored in buffer
	size_t blen; 	//length of buffer
	char *buf; 	//buffer
	char *b;	//start of stored data
}; // }}}

typedef struct varbuf vbuf;

#define VBUF_STATIC(x,y) static vbuf *x = NULL; if(!x) x = vbuf_alloc(y);

struct varbuf *vbuf_alloc( size_t len );
void vbuf_dump( vbuf *vb );
void vbuf_clear( struct varbuf *vbuf ); //ditch the data, keep the buffer
void vbuf_free( struct varbuf *vbuf );
void vbuf_resize( struct varbuf *vbuf, size_t len );
void vbuf_grow( struct varbuf *vbuf, size_t len ); // grow buffer by len bytes, data are preserved
void vbuf_strset( struct varbuf *vb, char *s ); // Store string s in vb
void vbuf_strnset( struct varbuf *vb, char *s, int n ); // Store string s in vb
void vbuf_set( struct varbuf *vbuf, void *data, size_t len );
void vbuf_strcat( vbuf *vb, char *str );
void vbuf_strncat( struct varbuf *vb, char *str, int len );
void vbuf_charcat( vbuf *vb, int ch );
void vbuf_strnprepend( struct varbuf *vb, char *str, int len ) ;
void vbuf_skip( vbuf *vbuf, size_t skip ); 
void vbuf_skipws( struct varbuf *vb ); 
void vbuf_append( struct varbuf *vbuf, void *data, size_t length );
void vbuf_overwrite( struct varbuf *vbdest, struct varbuf *vbsrc );
void vbuf_skipws( struct varbuf *vb );
void vbuf_printf( vbuf *vb, char *fmt, ... );
void vbuf_printfa( vbuf *vb, char *fmt, ... );
void vbuf_hexdump( vbuf *v, char *b, int start, int stop, int ascii );
int vbuf_catprintf( vbuf *vb, char *fmt, ... );
void vbuf_vprintf( vbuf *vb, char *fmt, va_list ap );
int vbuf_avail( vbuf *vb );
void vbuf_strtrunc( vbuf *v, int off ); // Drop chars [off..dlen]
int vs_last( vbuf *vb ); // returns the last character stored in a vbuf string

int parse_escaped_string( struct varbuf *vb, char *str, int len );

/*
 * Windows unicode output trash - this stuff sucks
 */

void unicode_init();
void unicode_close();
int utf16_write( FILE* stream, const void *buf, size_t count );
int utf16_fprintf( FILE* stream, const char *fmt, ... );
int utf16to8( char *inbuf_o, char *outbuf_o, int length );
int utf8to16( char *inbuf_o, size_t iblen, char *outbuf_o, size_t oblen);
int vb_utf8to16T( vbuf *bout, char *cin, size_t inlen );
int vb_utf16to8( vbuf *dest, char *buf, size_t len );
int iso8859_1to8( char *inbuf_o, char *outbuf_o, size_t length );
int utf8toascii( const char *inbuf_o, char *outbuf_o, size_t length );

/* duplicate str (allocate and copy in) */
char *str_dup( const char *str );

/* get user input */
int fmyinput(char *prmpt, char *ibuf, int maxlen);

/* Print len number of hexbytes */
void hexprnt(FILE *stream, unsigned char *bytes, int len);

/* HexDump all or a part of some buffer */
void hexdump(char *hbuf, int start, int stop, int ascii);

/* dump ascii hex in windoze format */
void winhex(FILE* stream, unsigned char *hbuf, int start, int stop, int loff);
void winhex8(FILE *stream, unsigned char *hbuf, int start, int stop, int loff );

void vbwinhex8(vbuf *vb, unsigned char *hbuf, int start, int stop, int loff );

/* general search routine, find something in something else */
int find_in_buf(char *buf, char *what, int sz, int len, int start);

/* Get INTEGER from memory. This is probably low-endian specific? */
int get_int( char *array );

/* Quick and dirty UNICODE to std. ascii */
void cheap_uni2ascii(char *src, char *dest, int l);

/* Quick and dirty ascii to unicode */
void cheap_ascii2uni(char *src, char *dest, int l);

/* increments *c until it isn't pointing at a ' ' */
void skipspace(char **c);

/* return the value of an ascii hex string */
int gethex(char **c);

struct uglyread *ugly_open( char *path ); // Horrible, just horrible
int ugly_eof( struct uglyread *uh );
void ugly_close( struct uglyread *th );
int find_nl( struct varbuf *vb ); // find newline of type type in b
int skip_nl( char *s ); // returns the width of the newline at s[0]
int ugly_readline( struct varbuf *vbo, struct uglyread *uh ); 
//int vb_readline( struct varbuf *vb, int *ctype, FILE *in ); // read *AT LEAST* one full line of data from in
int vb_skipline( struct varbuf *vb ); // in: vb->b == "stuff\nmore_stuff"; out: vb->b == "more_stuff"
/* Get a string of HEX bytes (space separated),
 * or if first char is ' get an ASCII string instead.  */
int gethexorstr(char **c, char *wb);
char *esc_index( char *s, int c ); // just like index(3), but works on strings with escape sequences
char *esc_rindex( char *s, int c ); // just like rindex(3), but works on strings with escape sequences

char *tok_esc_char( char *s, int *is_esc, int *c );
int vb_path_token( vbuf *tok, char **path ); // returns things like TOK_EMPTY, TOK_ERROR, complete list at top

void normalize_path( char *p, char *delim ); //in: p = "/a/broken\\\\path///with\\\\stuff"; out: p = "\\a\\broken\\path\\with\\stuff"
int gettoken( char *tok, int len, char **path, char delim ); // Path tokenizer: increments path, dumps token in tok
int debugit(char *buf, int sz); /* Simple buffer debugger, returns 1 if buffer dirty/edited */
#endif
