/****************************************************************************
*
*                            Open Watcom Project
*
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  Executable dumper mainline.
*
****************************************************************************/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <setjmp.h>
#include <unistd.h>

#include "wdglb.h"
#include "wdfunc.h"
#include "banner.h"


char  Fname[ _MAX_FNAME ];

/*
 * parse the executable
 */
static void dmp_exe( void )
/*************************/
{
    bool            dos_dmp;
    unsigned_32     len;

    Form = FORM_UNKN;
    Data_count = 0;
    Sizeleft = 0;
    Num_buff = -1;
    Byte_swap = FALSE;
    if( Options_dmp & EXE_INFO ) {
        dos_dmp = Dmp_dos_head();
        if( dos_dmp == 2 ) {
            Options_dmp |= HAS_NEW_EXE;
            if( Dmp_os2_head() ) {
                /* done */
            } else if( Dmp_386_head() ) {
                /* done */
            } else if( Dmp_pe_head() ) {
                /* done */
            } else {
                Wdputslc( "Invalid OS/2, PE header\n" );
            }
        } else if( dos_dmp == 3 ) {
            if( Dmp_d16m_head() ) {
                /* done */
            } else {
                Wdputslc( "No protected mode executable found\n" );
            }
        } else if( !dos_dmp ) {
            if( Dmp_os2_head() ) {
                /* done */
            } else if( Dmp_386_head() ) {
                /* done */
            } else if( Dmp_pe_head() ) {
                /* done */
            } else if( Dmp_dwarf() ) {
                /* done */
            } else if( Dmp_qnx_head() ) {
                /* done */
            } else if( Dmp_macho_head() ) {
                /* done */
            } else if( Dmp_elf_head() ) {
                /* done */
                // elf debug info has already been handled
                Options_dmp &= ~DEBUG_INFO;
            } else if( Dmp_phar_head() ) {
                /* done */
            } else if( Dmp_nlm_head() ) {
                /* done */
            } else if( Dmp_ar_head() ) {
                /* done */
            } else if( Dmp_lib_head() ) {
                /* done */
            } else if( Dmp_coff_head() ) {
                /* done */
            } else if( Dmp_d16m_head() ) {
                /* done */
            } else {
                Wdputs( Name );
                Wdputslc( " does not have recognized format\n" );
            }
        }
    }
    if( Options_dmp & DEBUG_INFO ) {
        Wdputslc( "\n" );
        if( Dmp_cv_head() ) {
            /* done */
        } else if( Dmp_hll_head() ) {
            /* done */
        } else if( Dmp_mdbg_head() ) {
            /* done */
        } else {
            Wdputslc( "No debugging information found\n" );
        }
    }
    if( Options_dmp & IMPORT_LIB ) {
        if( !Dmp_pe_tab() ) {
            Wdputslc( "Invalid PE file\n" );
        }
    }
    if( Options_dmp & IMPORT_TO_FMT ) {
      if( Import_format & IMPORTFMT_DEF ) {
          if( !Dmp_dll_exports_as_def() ) {
              Wdputslc( "No exports found\n" );
          }
      } else
      if( Import_format & IMPORTFMT_MAP ) {
          if( !Dmp_dll_exports_as_map() ) {
              Wdputslc( "No exports found\n" );
          }
      } else {
          Wdputslc( "Unsupported output format\n" );
      }
    }
    if( Options_dmp & DEBUG_TO_FMT ) {
        if( !Dmp_mdbg_head_as_map() ) {
            Wdputslc( "No debugging information found\n" );
        }
    }
    if( Options_dmp & BINARY_DMP ) {
        len = lseek( Handle, -Hexoff, SEEK_END );
        Wdputs( "offset = " );
        Puthex( Hexoff, 8 );
        Wdputs( ",  length = " );
        Puthex( len, 8 );
        Wdputslc( "\n" );
        Dmp_seg_data( Hexoff, len );
    }
    if( WSize ) {
        write( Lhandle, Write_buff, WSize );
    }
}

static int find_file( char * file_path )
/**************************************/
{
    int     access_rc;
    char    drive[ _MAX_DRIVE ];
    char    dir[ _MAX_DIR ];
    char    given_ext[ _MAX_EXT ];

    _splitpath( file_path, drive, dir, Fname, given_ext );
    /* check if the given file name exists */
    access_rc = access( file_path, R_OK );
    if( access_rc == 0 ) {
        return( TRUE );
    } else {
        return( FALSE );
    }
}

/*
 * handles file to be dumped and .lst file if required
 */
static int open_files( void )
/***************************/
{
    int                 ret;

    Handle = open( Name, O_RDONLY | O_BINARY, 0 );
    if( Handle != -1 ) {
        ret = setjmp( Se_env );
        if( ret == 0 ) {
            dmp_exe();
        }
        close( Handle );
        return( ret );
    }
    return( 1 );
}

/*
 * The copyright banner
 */
static void wbanner( void )
/*************************/
{
    Wdputs( banner1w( "Executable Image Dump Utility", BAN_VER_STR ) "\n" );
    Wdputs( banner2a() "\n" );
    Wdputs( banner3 "\n" );
    Wdputs( banner3a "\n\n" );
}

/*
 * Usage
 */
static void usage( void )
/***********************/
{
    Wdputs( "Usage: wdump [-?abdefipqrsx] [-A<num>] [-B<off>] [-D<opt>] [-S<num>] <file>\n" );
    Wdputs( "  <file> is a DOS EXE file, a Windows or OS/2 executable or DLL,\n" );
    Wdputs( "            a PharLap executable, NLM, a QNX executable,\n" );
    Wdputs( "            an ELF executable, shared library or object file,\n" );
    Wdputs( "            or a COFF object.\n" );
    Wdputs( "  options:\n" );
    Wdputs( "        -a causes all segment, resource and fixup info to be dumped\n" );
    Wdputs( "        -A<segnum> like -a but only applies to segment <segnum>\n" );
    Wdputs( "        -b causes binary dump of the entire file\n" );
    Wdputs( "        -B<hexoff> causes binary dump beginning at offset in hex\n" );
    Wdputs( "        -d causes debugging information to be dumped in text form\n" );
    Wdputs( "        -D<opts> controls debugging information to be dumped\n" );
    Wdputs( "           a : show addr infomation\n" );
    Wdputs( "           g : show global infomation\n" );
    Wdputs( "           l : show locals (only if m specified)\n" );
    Wdputs( "           m : show module informatiom\n" );
    Wdputs( "           n : show line numbers (only if m specified)\n" );
    Wdputs( "           t : show types (only if m specified)\n" );
    Wdputs( "           x : show all information\n" );
    Wdputs( "        -e causes executable information to be dumped as well\n" );
    Wdputs( "        -f causes fixup information to be dumped\n" );
    Wdputs( "        -i dump export information for PE DLLs\n" );
    Wdputs( "        -p causes LE/LX page map to be dumped\n" );
    Wdputs( "        -q quiet dump - don't write banner\n" );
    Wdputs( "        -r causes more resource information to be dumped\n" );
    Wdputs( "        -s causes segments' data to be dumped\n" );
    Wdputs( "        -S<segnum> like -s but only applies to segment <segnum>\n" );
    Wdputs( "        -x dump export information for PE/NE/LX DLLs in .DEF format\n" );
    Wdputs( "        -X<opts> dump export information for DLLs into chosen format\n" );
    Wdputs( "           d : .DEF format          m : .MAP format\n" );
    Wdputs( "           o : output ordinals      a : output entry address\n" );
    Wdputs( "        -n dump debugging information into .MAP format\n" );
    Wdputs( "        -N<opts> dump debugging info into .MAP with chosen options\n" );
    Wdputs( "           m : output module index  k : output entry kind\n" );
    Wdputs( "           s : strip names to basic form\n" );
}

/*
 * Analysis of command line debug data dump options.
 */
static void debug_opts( char ch )
/*******************************/
{
    switch( tolower( ch ) ) {
    case 'x':
        Debug_options = 0xff;
        break;
    case 'm':
        Debug_options |= MODULE_INFO;
        break;
    case 'g':
        Debug_options |= GLOBAL_INFO;
        break;
    case 'a':
        Debug_options |= ADDR_INFO;
        break;
    case 'n':
        Debug_options |= LINE_NUMS;
        break;
    case 'l':
        Debug_options |= LOCALS;
        break;
    case 't':
        Debug_options |= TYPES;
        break;
    }
}

/*
 * Analysis of command line export information dump options.
 */
static void import_opts( char ch )
/*******************************/
{
    switch( tolower( ch ) ) {
    case 'd':
        Import_format |= IMPORTFMT_DEF;
        Import_format &= ~IMPORTFMT_MAP;
        break;
    case 'm':
        Import_format |= IMPORTFMT_MAP;
        Import_format &= ~IMPORTFMT_DEF;
        Import_format |= IMPORT_INCADDR;
        break;
    case 'a':
        Import_format |= IMPORT_INCADDR;
        break;
    case 'o':
        Import_format |= IMPORT_INCORDN;
        break;
    }
}

/*
 * Analysis of command line debug information dump to DEF/MAP options.
 */
static void dbgimp_opts( char ch )
/*******************************/
{
    switch( tolower( ch ) ) {
    case 'm':
        DbgImp_format |= DBGIMP_INCMODL;
        break;
    case 'k':
        DbgImp_format |= DBGIMP_INCKIND;
        break;
    case 's':
        DbgImp_format |= DBGIMP_NMSTRIP;
        break;
    }
}

static int parse_options( int argc, char * const *argv )
/******************************************************/
{
    int     c;
    char    *arg;

    Options_dmp = EXE_INFO;
    Import_format = IMPORTFMT_DEF | IMPORT_INCORDN;
    DbgImp_format = 0;
    Segspec = 0;
    Hexoff = 0;

    while( 1 ) {
        while( (c = getopt( argc, argv, ":aA:bB:dD:efipqrsS:xX:nN:" )) != -1 ) {
            switch( c ) {
            case 'A':
                Options_dmp |= FIX_DMP | PAGE_DMP | RESRC_DMP | EXE_INFO | DOS_SEG_DMP | OS2_SEG_DMP;
                Segspec = atoi( optarg );
                if( Segspec == 0 ) {
                    Options_dmp &= ~OS2_SEG_DMP;
                } else {
                    Options_dmp &= ~DOS_SEG_DMP;
                }
                break;
            case 'a':
                Options_dmp |= FIX_DMP | PAGE_DMP | RESRC_DMP | EXE_INFO | DOS_SEG_DMP | OS2_SEG_DMP;
                break;
            case 'B':
                Hexoff = strtol( optarg, NULL, 16 );
                /* fall through */
            case 'b':
                Options_dmp |= BINARY_DMP;
                Options_dmp &= ~EXE_INFO;
                break;
            case 'D':
                Debug_options = 0;
                arg = optarg;
                while( islower( *arg ) || isupper( *arg ) ) {
                    debug_opts( *arg++ );
                }
                /* fall through */
            case 'd':
                Options_dmp |= DEBUG_INFO;
                Options_dmp &= ~EXE_INFO;
                break;
            case 'e':
                Options_dmp |= EXE_INFO;
                break;
            case 'f':
                Options_dmp |= EXE_INFO;
                Options_dmp |= FIX_DMP;
                break;
            case 'i':
                Options_dmp |= IMPORT_LIB;
                Options_dmp |= QUIET;
                Options_dmp &= ~EXE_INFO;
                break;
            case 'p':
                Options_dmp |= EXE_INFO | PAGE_DMP;
                break;
            case 'q':
                Options_dmp |= QUIET;
                break;
            case 'r':
                Options_dmp |= EXE_INFO | RESRC_DMP;
                break;
            case 'S':
                Options_dmp |= EXE_INFO | DOS_SEG_DMP | OS2_SEG_DMP;
                Segspec = atoi( optarg );
                if( Segspec == 0 ) {
                    Options_dmp &= ~OS2_SEG_DMP;
                } else {
                    Options_dmp &= ~DOS_SEG_DMP;
                }
                break;
            case 's':
                Options_dmp |= EXE_INFO | DOS_SEG_DMP | OS2_SEG_DMP;
                break;
            case 'X':
                Import_format = 0;
                arg = optarg;
                while( islower( *arg ) || isupper( *arg ) ) {
                    import_opts( *arg++ );
                }
                /* fall through */
            case 'x':
                Options_dmp |= IMPORT_TO_FMT;
                Options_dmp |= QUIET;
                Options_dmp &= ~EXE_INFO;
                break;
            case 'N':
                DbgImp_format = 0;
                arg = optarg;
                while( islower( *arg ) || isupper( *arg ) ) {
                    dbgimp_opts( *arg++ );
                }
                /* fall through */
            case 'n':
                Options_dmp |= DEBUG_TO_FMT;
                Options_dmp |= QUIET;
                Options_dmp &= ~EXE_INFO;
                break;
            case ':':
                Wdputs( "wdump: option requires argument: -" );
                Wdputc( optopt );
                Wdputslc( "\n" );
                return( 1 );
            case '?':
                usage();
                return( 1 );
            default:
                Wdputs( "wdump: invalid option: -" );
                Wdputc( c );
                Wdputslc( "\n" );
                return( 1 );
            }
        }
        if( optind < argc ) {
            if( Name == NULL ) {
                Name = argv[optind++];
            } else {
                Wdputs( "wdump: multiple pathnames not accepted\n" );
                return( 1 );
            }
        } else {
            break;
        }
    }
    return( 0 );
}

/*
 * The Increasingly More Misnamed Segmented .EXE File Header Dump Utility
 * for WLINK and WD (nee WVIDEO)
 */
int main( int argc, char * const *argv )
/**************************************/
{
    int     found_file;

    WSize = 0;
    Write_buff[WSize] = 0;
    if( parse_options( argc, argv ) ) {
        return( 1 );
    }
    if( !(Options_dmp & QUIET) ) {
        wbanner();
    }
    if( Name == NULL || *Name == '\0' ) {
        usage();
        return( 1 );
    }
    found_file = find_file( Name );
    if( found_file ) {
        return( open_files() );
    }
    Wdputs( "Could not open '" );
    Wdputs( Name );
    Wdputs( "': " );
    Wdputslc( strerror( errno ) );
    Wdputslc( "\n" );
    return( 1 );
}
