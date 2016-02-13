// {{{ #include <stuff>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "libwab.h"
#include "tools.h"
// }}}

void use() // {{{
{
  fprintf( stderr, 
    "Use:  wabread [options] <filename.wab>\n"
    "\n"
    "  Options:\n"
    "   -d #        set debugging (logical or 1,2,3,4...)\n"
    "   -h          heuristic record dump: attempt to recover data\n"
    "                 from a broken .wab file and/or deleted records.\n"
    "   -u          deprecated enable unicode flag (is always on now).\n"
  );
}
// }}}

#define MODE_NORMAL 0
#define MODE_HEURISTIC 1


//int do_heuristic( char *path );

int mode;

struct wab_header wabhead;

void parse_cmdline( int *argc, char ***argv );

//char *filename = NULL;

int main( int argc, char **argv ) // {{{
{
	int i;
	struct wab_handle *wh;
	VBUF_STATIC( fname, 10 );

	if( argc < 2 ) {
		use();
		return 1;
	}

	parse_cmdline( &argc, &argv );

	if( argc != 1 ) {
		fprintf(stderr, "Please specify a file.\n");
		exit(1);
	}

	vbuf_strset( fname, argv[0] );

	dump_ldif_header(stdout);

	switch( mode ) {
		case MODE_NORMAL:
		{
			if( NULL == ( wh = open_wab( fname->b ) ) ) {
				fprintf( stderr, "Error opening %s\n", fname->b );
				return 1;
			}

			DEBUG( DB_DATA_DUMP, dump_wab_header( wh ););

			DEBUG( DB_DATA_DUMP,
			for( i=0; i<TABLE_COUNT; i++) {
				fprintf( stderr, "\nDumping lookup table %d\n", i );
				dump_table( i, wh );
			}
			);
			output_records( wh );
			close_wab( wh );
			break;
		}

		case MODE_HEURISTIC:
			return do_heuristic( fname->b );
		break;

		default:
			fprintf( stderr, "ERROR: Unknown mode (this should never happen).\n");
			exit(1);
	}

	return 0;
}
// }}}
void parse_cmdline( int *argc, char ***argv ) // {{{ parse leading arguments
{
        extern int optind;
        extern char* optarg;
        char c;

        char *options = "hcud:";

        while((c=getopt(*argc,*argv,options)) > 0) {
                switch(c) {
                        case 'h': mode = MODE_HEURISTIC; break;
			case 'u': ;      break;
                        case 'd':
				if( 1 != sscanf( optarg, "%d", &dodebug ) ) {
					fprintf(stderr, "Error reading parameter to -d \"%s\", expected integer.\n", optarg);
					exit(1);
				}
				break;
                        default:
                                  fprintf(stderr, "Unrecognized option %c.  Run with no arguments for help.\n", c);
                                  exit(1);
                }
        }

        *argc -= optind;
        *argv += optind;

} // }}}
