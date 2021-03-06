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
* Description:  DWARF dump routines.
*
****************************************************************************/


#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "wdglb.h"
#include "dumpwv.h"
#include "wdfunc.h"


static  char    *mdh_msg[] = {
    "1EXE major                 = ",
    "1EXE minor                 = ",
    "1OBJ major                 = ",
    "1OBJ minor                 = ",
    "2Language data size        = ",
    "2Segment table size        = ",
    "4Total size of debug info  = ",
    NULL
};

typedef struct {
    char        signature[4];
    unsigned_32 vendor_id;
    unsigned_32 info_type;
    unsigned_32 info_size;
} debug_header;

extern char  Fname[ _MAX_FNAME ];

static void set_sects( void )
/***************************/
{
    unsigned_32         sectsizes[DR_DEBUG_NUM_SECTS];
    unsigned_32         sections[DR_DEBUG_NUM_SECTS];
    int                 i;

    memset( sections, 0, DR_DEBUG_NUM_SECTS * sizeof(unsigned_32) );
    memset( sectsizes, 0, DR_DEBUG_NUM_SECTS * sizeof(unsigned_32) );

    Wread( &sections[DW_DEBUG_INFO], sizeof(unsigned_32) );
    Wread( &sectsizes[DW_DEBUG_INFO], sizeof(unsigned_32) );
    Wread( &sections[DW_DEBUG_REF], sizeof(unsigned_32) );
    Wread( &sectsizes[DW_DEBUG_REF], sizeof(unsigned_32) );
    Wread( &sections[DW_DEBUG_ABBREV], sizeof(unsigned_32) );
    Wread( &sectsizes[DW_DEBUG_ABBREV], sizeof(unsigned_32) );
    Wread( &sections[DW_DEBUG_LINE], sizeof(unsigned_32) );
    Wread( &sectsizes[DW_DEBUG_LINE], sizeof(unsigned_32) );
    Wread( &sections[DW_DEBUG_MACINFO], sizeof(unsigned_32) );
    Wread( &sectsizes[DW_DEBUG_MACINFO], sizeof(unsigned_32) );

    for( i = 0; i < DR_DEBUG_NUM_SECTS; i += 1 ) {
        Sections[i].cur_offset = 0;
        Sections[i].max_offset = sectsizes[i];

        if( sectsizes[i] != 0 ) {
            Wlseek( sections[i] );
            Sections[i].data = Wmalloc( sectsizes[i] );
            if( Sections[i].data == NULL ) {
                Wdputslc( "Not enough memory\n" );
                exit(1);
            }
            Wread( Sections[i].data, sectsizes[i] );
        }
    }
}

/*
 * Parse OS2 LE/LX executable header.
 */
static bool parse_os2_linear_exe_head( struct dos_exe_header *dos_head, struct os2_flat_header *os2_head )
/*********************************************/
{
    unsigned_32 new_exe_off;
    new_exe_off = 0;
    Wlseek( 0 );
    Wread( dos_head, sizeof( struct dos_exe_header ) );
    if( dos_head->signature == DOS_SIGNATURE ) {
        if( dos_head->reloc_offset == OS2_EXE_HEADER_FOLLOWS ) {
            Wlseek( OS2_NE_OFFSET );
            Wread( &new_exe_off, sizeof( new_exe_off ) );
        }
    }
    // MZ stub is optional
    Wlseek( new_exe_off );
    Wread( os2_head, sizeof( struct os2_flat_header ) );
    if( os2_head->signature == OSF_FLAT_SIGNATURE ||
        os2_head->signature == OSF_FLAT_LX_SIGNATURE ) {
        return ( TRUE );
    }
    return( FALSE );
}

/*
 * Parse OS2 LE/LX executable header.
 */
static bool parse_os2_linear_exe_debug_head( void )
/*********************************************/
{
    if( !parse_os2_linear_exe_head( &Dos_head, &Os2_386_head ) ) {
        return( FALSE );
    }
    if( Os2_386_head.debug_len == 0 ) {
        return( FALSE );
    }
    if( !Parse_elf_header( &Elf_head, Os2_386_head.debug_off ) ) {
        return( FALSE );
    }
    return( TRUE );
}

static bool dump_os2_linear_exe_debug_head( void )
/***************************/
{
    if( Os2_386_head.debug_len ) {
        Dmp_elf_header( &Elf_head, Os2_386_head.debug_off );
        return( TRUE );
    } else {
        Wdputslc( "No OS/2 LE or LX debugging info\n" );
    }
    return( FALSE );
}

/*
 * Check whether the debug header is valid.
 */
static bool is_valid_master_dbg_head( master_dbg_header *mdh )
{
    return ( mdh->signature == VALID_SIGNATURE );
}

/*
 * Check whether the debug header is valid and supported.
 */
static bool is_supported_master_dbg_head( master_dbg_header *mdh )
{
    return ( mdh->signature == VALID_SIGNATURE &&
        mdh->exe_major_ver == EXE_MAJOR_VERSION &&
        (signed)mdh->exe_minor_ver <= EXE_MINOR_VERSION &&
        mdh->obj_major_ver == OBJ_MAJOR_VERSION &&
        mdh->obj_minor_ver <= OBJ_MINOR_VERSION );
}

/*
 * Parse dynamic lists within the Master Debug Header.
 */
static bool parse_master_dbg_header( master_dbg_header *mdh )
/*********************************************/
{
    Curr_sectoff = lseek( Handle, 0, SEEK_END );
    Wlseek( Curr_sectoff -(int)sizeof( master_dbg_header ) );
    Wread( mdh, sizeof( master_dbg_header ) );

    if( !is_valid_master_dbg_head( mdh ) )
        return( FALSE );

    if( !is_supported_master_dbg_head( mdh ) )
        Wdputslc( "Unsupported version in Master Debug Header; continuing anyway\n" );

    Curr_sectoff -= (long)mdh->debug_size;
    Wlseek( Curr_sectoff );

    Lang_lst = Wmalloc( mdh->lang_size );
    Wread( Lang_lst, mdh->lang_size );
    Curr_sectoff += (long)mdh->lang_size;

    Wbuff = Wmalloc( MAX_BUFF );
    Wread( Wbuff, mdh->segment_size );
    Curr_sectoff += (long)mdh->segment_size;

    return( TRUE );
}

/*
 * Dump the Master Debug Header, with dynamic lists.
 */
static void dmp_master_dbg( master_dbg_header *mdh )
/*********************************************/
{
    int                     i;

    Banner( "Master Debug Info" );
    Dump_header( (char *)&mdh->exe_major_ver, mdh_msg );
    Wdputslc( "\n" );

    i = 0;
    Wdputslc( "Languages\n" );
    Wdputslc( "=========\n" );
    while( i < mdh->lang_size ) {
        Wdputs( &Lang_lst[i] );
        Wdputslc( "\n" );
        i += strlen( &Lang_lst[i] ) + 1;
    }
    Wdputslc( "\n" );

    Wdputslc( "Segments\n" );
    Wdputslc( "========\n" );
    i = 0;
    while( i < mdh->segment_size ) {
        Puthex( *(unsigned_16 *)&Wbuff[i], 4 );
        Wdputslc( "\n" );
        i += sizeof( unsigned_16 );
    }
    Wdputslc( "\n" );
}

/*
 * Parse TIS Debug Header.
 */
static bool parse_tis_dbg_header( debug_header *dbg, unsigned_32 offs )
/*********************************************/
{
    const char *signature = "TIS";
    Wlseek( offs );
    Wread( dbg, sizeof( debug_header ) );
    if( memcmp( dbg->signature, signature, 4 ) != 0 )
        return( FALSE );
    return( TRUE );
}

/*
 * Dump the Debug Header, if any.
 */
bool Dmp_mdbg_head( void )
/************************/
{
    debug_header        dbg;
    unsigned_16         cnt;
    master_dbg_header   mdh;

    if( parse_master_dbg_header( &mdh ) ) {
        dmp_master_dbg( &mdh );
        Dump_section();
        return( 1 );
    }
    if( Parse_elf_header( &Elf_head, 0 ) ) {
        // Handle ELF executables without TIS signature
        Dmp_elf_header( &Elf_head, 0 );
        return( 1 );
    }
    if( parse_os2_linear_exe_debug_head( ) ) {
        // Handle NE/LX executables without TIS signature
        dump_os2_linear_exe_debug_head( );
        return( 1 );
    }
    { // Handle executables with TIS signature
        cnt = 0;
        while( parse_tis_dbg_header( &dbg, Curr_sectoff -(int)sizeof( debug_header ) ) ) {
            cnt++;
            Wdputs( "size of information = " );
            Puthex( dbg.info_size, 4 );
            Wdputslc( "\n" );

            Curr_sectoff -= dbg.info_size;

            if( Parse_elf_header( &Elf_head, Curr_sectoff ) ) {
                Dmp_elf_header( &Elf_head, Curr_sectoff );
            } else {
                Banner( "Data" );
                Dmp_seg_data( Curr_sectoff, dbg.info_size - sizeof( debug_header ) );
            }
        }
        if( cnt > 0 )
            return( 1 );
    }
    return( 0 );
}

/*
 * Dump the Debug Header, (if any) into .MAP format.
 */
bool Dmp_mdbg_head_as_map( void )
/************************/
{
    master_dbg_header   mdh;

    Wdputs( "MAP file for module '" );
    Wdputs( Fname );
    Wdputslc( "'\n" );

    if( parse_master_dbg_header( &mdh ) ) {
        Dump_section_as_map();
        return( 1 );
    } else {
        // This case is not handled yet for .MAP dumping
        Wdputs( "Unsupported format in Master Debug Header" );
    }
}

/*
 * verify browser identification bytes
 */
bool Dmp_dwarf( void )
/********************/
{
    char        *mbrHeaderString = "WBROWSE";
    int         mbrHeaderStringLen = 7;
    unsigned    mbrHeaderSignature = 0xcbcb;
    char        mbrHeaderVersion = '1';
    unsigned_16 signature;
    char        buf[7];

    Wlseek( 0 );
    Wread( &signature, sizeof(unsigned_16) );
    if( signature != mbrHeaderSignature ){
        return( 0 );
    }
    Wread( buf, mbrHeaderStringLen );
    if( memcmp( buf, mbrHeaderString, mbrHeaderStringLen ) != 0 ) {
        return( 0 );
    }
    Wread( buf, sizeof(char) );
    if( buf[0] != mbrHeaderVersion ) {
        return( 0 );
    }
    set_sects();
    Dump_all_sections();
    return( 1 );
}
