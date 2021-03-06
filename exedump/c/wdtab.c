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
* Description:  NE/LE/LX name tables processing.
*
****************************************************************************/


#include <stdio.h>
#include <setjmp.h>
#include <unistd.h>
#include "walloca.h"

#include "wdglb.h"
#include "wdfunc.h"


struct  int_entry_pnt     *Entry_pnts = NULL;

/*
 * Read a resident/nonresident name and ordinal
 */
static unsigned_8 read_res_nonres_nam( char *name, unsigned_16 *ordinal )
/***********************************************************************/
{
    unsigned_8              string_len;

    Wread( &string_len, sizeof( string_len ) );
    if( string_len ) {
        Wread( name, string_len );
        Wread( ordinal, sizeof( unsigned_16 ) );
    }
    name[ string_len ] = '\0';
    return( string_len );
}

/*
 * Dump a resident/nonresident name
 */
static unsigned_8 dmp_res_nonres_nam( void )
/******************************************/
{
    char                    resident[MAX_EXPORT_NAME_LEN];
    unsigned_16             entry_index;
    unsigned_8              len;

    len = read_res_nonres_nam( resident, &entry_index );
    if( len ) {
        if( Form == FORM_NE ) {
            Wdputs( resident );
            Dmp_ordinal( entry_index );
        } else {
            Wdputs( "ordinal " );
            Puthex( entry_index, 4 );
            Wdputs( ": " );
            Wdputs( resident );
        }
        Wdputslc( "\n" );
    }
    return( len );
}

/*
 * Dump the Resident or Nonresident Names Tables
 */
static void dmp_res_nonres_tab( unsigned_32 res_nam_tab )
/*******************************************************/
{
    if( res_nam_tab <= New_exe_off ) return;
    Wlseek( res_nam_tab );
    while( dmp_res_nonres_nam() )
        ;
}

/*
 * Dump the module reference table
 */
static void dmp_imp_tab( unsigned_32 proc_off, unsigned_32 size_proc )
/********************************************************************/
{
    unsigned_8              string_len;
    unsigned_16             size;
    char                    *imp_nam;

    Wlseek( proc_off );
    for( size = 0; size < size_proc; size += string_len + 1 ) {
        Wread( &string_len, sizeof( unsigned_8 ) );
        imp_nam = Wmalloc( string_len + 1 );
        Wread( imp_nam, string_len );
        imp_nam[ string_len ] = '\0';
        Wdputs( imp_nam );
        Wdputslc( "\n" );
    }
}

/*
 * Dump the module reference table
 */
static void dmp_mod_ref_tab( unsigned_32 mod_ref, unsigned_16 num_mod_ref )
/*************************************************************************/
{
    unsigned_16                     *mod_ref_tab;
    unsigned_16                     size_mod_ref;
    unsigned_16                     ref_num;
    char                            *imp_nam;
    unsigned_32                     imp_off;
    unsigned_8                      string_len;

    if( num_mod_ref == 0 ) {
        return;
    }
    Wdputslc( "\n" );
    Banner( "Module Reference Table" );
    Wlseek( mod_ref );
    size_mod_ref = num_mod_ref * sizeof( unsigned_16 );
    mod_ref_tab = Wmalloc( size_mod_ref );
    Wread( mod_ref_tab, size_mod_ref );
    Int_mod_ref_tab = Wmalloc( num_mod_ref * sizeof( unsigned_8 * ) );
    for( ref_num = 0; ref_num != num_mod_ref; ++ref_num ) {
        imp_off = New_exe_off + mod_ref_tab[ ref_num ] + Os2_head.import_off;
        Wlseek( imp_off );
        Wread( &string_len, sizeof( unsigned_8 ) );
        imp_nam = Wmalloc( string_len + 1 );
        Wread( imp_nam, string_len );
        imp_nam[ string_len ] = '\0';
        Wdputs( imp_nam );
        Wdputslc( "\n" );
        Int_mod_ref_tab[ ref_num ] = imp_nam;
    }
    free( mod_ref_tab );
}

/*
 * Dump the Imported Names Tables
 */
static void dmp_import_tab( unsigned_32 imp_nam_tab )
/***************************************************/
{
    unsigned_8                      string_len;
    char                            *resident;

    Wlseek( imp_nam_tab );
    Wread( &string_len, sizeof( unsigned_8 ) );
    if( string_len == 0 ) {
        return;
    }
    Wdputslc( "\n" );
    Banner( "Imported Name Table" );
    for( ;; ) {
        resident = alloca( string_len );
        if( resident == NULL ) {
            Wdputslc( "Error! Dynamic memory exhausted.\n" );
            longjmp( Se_env, 1 );
        }
        Wread( resident, string_len );
        resident[ string_len ] = '\0';
        Wdputs( resident );
        Wdputslc( "\n" );
        Wread( &string_len, sizeof( unsigned_8 ) );
        if( string_len == 0 ) {
            return;
        }
    }
}

/*
 * Dump the Entry Table
 */
static void dmp_ent_type( unsigned_8 type, unsigned ordinal )
/***********************************************************/
{
    flat_bundle_entry32     ent_bund32;
    flat_bundle_entry16     ent_bund16;
    flat_bundle_gate16      gate_bund;
    flat_bundle_entryfwd    ent_bund_fwd;

    switch( type ) {
    case FLT_BNDL_EMPTY:
        break;
    case FLT_BNDL_ENTRY16:
        Wread( &ent_bund16, sizeof( flat_bundle_entry16 ) );
        Wdputslc( "\nordinal = " );
        Puthex( ordinal, 4 );
        Wdputs( "   flags = " );
        Puthex( ent_bund16.e32_flags, 2 );
        Wdputs( "   offset = " );
        Puthex( ent_bund16.e32_offset, 4 );
        if( ent_bund16.e32_flags & ENTRY_EXPORTED ) {
            Wdputs( "  EXPORTED" );
        }
        if( ent_bund16.e32_flags & ENTRY_SHARED ) {
            Wdputs( "  SHARED DATA" );
        }
        break;
    case FLT_BNDL_GATE16:
        Wread( &gate_bund, sizeof( flat_bundle_gate16 ) );
        Wdputslc( "\nordinal = " );
        Puthex( ordinal, 4 );
        Wdputs( "   flags = " );
        Puthex( gate_bund.e32_flags, 2 );
        Wdputs( "   offset = " );
        Puthex( gate_bund.offset, 4 );
        Wdputs( "   callgate = " );
        Puthex( gate_bund.callgate, 4 );
        if( gate_bund.e32_flags & ENTRY_EXPORTED ) {
            Wdputs( "  EXPORTED" );
        }
        if( gate_bund.e32_flags & ENTRY_SHARED ) {
            Wdputs( "  SHARED DATA" );
        }
        break;
    case FLT_BNDL_ENTRY32:
        Wread( &ent_bund32, sizeof( flat_bundle_entry32 ) );
        Wdputslc( "\nordinal = " );
        Puthex( ordinal, 4 );
        Wdputs( "   flags = " );
        Puthex( ent_bund32.e32_flags, 2 );
        Wdputs( "   offset = " );
        Puthex( ent_bund32.e32_offset, 8 );
        if( ent_bund32.e32_flags & ENTRY_EXPORTED ) {
            Wdputs( "  EXPORTED" );
        }
        if( ent_bund32.e32_flags & ENTRY_SHARED ) {
            Wdputs( "  SHARED DATA" );
        }
        break;
    case FLT_BNDL_ENTRYFWD:
        Wread( &ent_bund_fwd, sizeof( flat_bundle_entryfwd ) );
        Wdputslc( "\nordinal = " );
        Puthex( ordinal, 4 );
        Wdputs( "   flags = " );
        Puthex( ent_bund_fwd.e32_flags, 2 );
        Wdputs( "   module ordinal = " );
        Puthex( ent_bund_fwd.modord, 4 );
        Wdputs( "   offset or ordinal = " );
        Puthex( ent_bund_fwd.value, 8 );
        if( ent_bund_fwd.e32_flags & ENTRY_EXPORTED ) {
            Wdputs( "  EXPORTED" );
        }
        if( ent_bund_fwd.e32_flags & ENTRY_SHARED ) {
            Wdputs( "  SHARED DATA" );
        }
        break;
    }
}

/*
 * Dump the Entry Table
 */
static void dmp_ent_tab( unsigned_32 ent_tab )
/********************************************/
{
    flat_null_prefix        ent_bund_pfx;
    unsigned                ordinal = 1;
    unsigned_16             object;
    unsigned                i;

    Wlseek( ent_tab );
    Wread( &ent_bund_pfx, sizeof( ent_bund_pfx ) );
    if( !ent_bund_pfx.b32_cnt ) {
        return;
    }
    Wdputslc( "\n" );
    Banner( "Entry Point Table" );
    for( ; ent_bund_pfx.b32_cnt != 0; ) {
        Wdputs( "\nnumber of entries in bundle = " );
        Puthex( ent_bund_pfx.b32_cnt, 2 );
        Wdputslc( "\ntype = " );
        Puthex( ent_bund_pfx.b32_type, 2 );
        if( ent_bund_pfx.b32_type != FLT_BNDL_EMPTY ) {
            Wread( &object, sizeof( object ) );
            Wdputslc( "\nobject number = " );
            Puthex( object, 4 );
        }
        Wdputslc( "\n" );
        for( i = 0; i < ent_bund_pfx.b32_cnt; ++i ) {
            dmp_ent_type( ent_bund_pfx.b32_type, ordinal++ );
        }
        Wdputslc( "\n" );
        Wread( &ent_bund_pfx, sizeof( ent_bund_pfx ) );
    }
    Wdputslc( "\n" );
}

/*
 * allocate a new int_entry_pnt structure
 */
static struct int_entry_pnt *new_ent_pnt( void )
/**********************************************/
{
    struct int_entry_pnt    *new_ent;

    new_ent = Wmalloc( sizeof( struct int_entry_pnt ) );
    new_ent->next = Entry_pnts;
    Entry_pnts = new_ent;
    return( new_ent );
}

static void *dmp_fixed_seg_ent_pnts( unsigned_16 num_ent_pnts,
        void *ent_pnts, unsigned_16 ent_pnt_index, unsigned_16 seg_num )
/**********************************************************************/
{
    struct fixed_record             *fix_ent;
    struct int_entry_pnt            *new_pnt;

    fix_ent = ent_pnts;
    for( ; num_ent_pnts != 0; num_ent_pnts-- ) {
        new_pnt = new_ent_pnt();
        new_pnt->ordinal = ent_pnt_index++;
        new_pnt->seg_num = seg_num;
        new_pnt->offset = fix_ent->entry;
        new_pnt->ent_flag = fix_ent->info;
        ++fix_ent;
    }
    return( fix_ent );
}

static void *dmp_movable_seg_ent_pnts( unsigned_16 num_ent_pnts,
                        void *ent_pnts, unsigned_16 ent_pnt_index )
/*****************************************************************/
{
    struct movable_record           *mov_ent;
    struct int_entry_pnt            *new_pnt;

    mov_ent = ent_pnts;
    for( ; num_ent_pnts != 0; num_ent_pnts-- ) {
        new_pnt = new_ent_pnt();
        new_pnt->ordinal = ent_pnt_index++;
        new_pnt->ent_flag = mov_ent->info;
        new_pnt->seg_num = mov_ent->entrynum;
        new_pnt->offset = mov_ent->entry;
        ++mov_ent;
    }
    return( mov_ent );
}

/*
 * Parses the Entry Table of NE file. Is compatible ONLY with NE files.
 * Stores the data in Entry_pnts structure.
 */
static void parse_ne_entry_table( unsigned_32 ent_tab, unsigned_16 ent_tab_len )
/*********************************************************************/
{
    struct bundle_prefix    *ent_bund;
    struct bundle_prefix    *init_ent_bund;
    unsigned_16             ent_pnt_index;
    unsigned_16             num_ent_pnts;

    if( ent_tab_len == 0 ) {
        return;
    }
    Wlseek( ent_tab );
    ent_bund = Wmalloc( ent_tab_len );
    init_ent_bund = ent_bund;
    Wread( ent_bund, ent_tab_len );
    ent_pnt_index = 1;
    for( ; ent_bund->number != 0; ) {
        num_ent_pnts = ent_bund->number;
        switch( ent_bund->type ) {
        case 0:                   /* just incrementing entry table index */
            ++ent_bund;
            break;
        case MOVABLE_ENTRY_PNT:       /* movable segment                 */
            ent_bund = dmp_movable_seg_ent_pnts( num_ent_pnts,
                ++ent_bund, ent_pnt_index );
            break;
        default:                  /* fixed segment                       */
            ent_bund = dmp_fixed_seg_ent_pnts( num_ent_pnts,
                ++ent_bund, ent_pnt_index, ent_bund->type );
            break;
        }
        ent_pnt_index += num_ent_pnts;
    }
    free( init_ent_bund );
}

static void dmp_an_ord( struct int_entry_pnt *find )
/**************************************************/
{
    unsigned_16     flag;

    Wdputc( '.' );
    Putdec( find->ordinal );
    Wdputs( " seg " );
    Puthex( find->seg_num, 4 );
    Wdputs( " off " );
    Puthex( find->offset, 4 );
    Wdputs( " parm " );
    flag = find->ent_flag;
    Puthex( flag >> IOPL_WORD_SHIFT, 4 );
    if( flag & ENTRY_EXPORTED ) {
        Wdputs( " EXPORTED" );
    }
    if( flag & ENTRY_SHARED ) {
        Wdputs( "|SHAREDATA" );
    }
}

struct int_entry_pnt *get_entry_point_by_ordinal( unsigned_16 ord )
{
    struct int_entry_pnt    *find;
    for( find = Entry_pnts; find != NULL; find = find->next ) {
        if( find->ordinal == ord ) {
            return find;
        }
    }
    return NULL;
}

/*
 * Dumps an entry point recognized by given ordinal.
 */
void Dmp_ordinal( unsigned_16 ord )
/*********************************/
{
    struct int_entry_pnt    *find;
    find = get_entry_point_by_ordinal( ord );
    if ( find == NULL ) {
        Wdputs( " unknown ordinal " );
        Puthex( ord, 4 );
        return;
    }
    dmp_an_ord( find );
}

/*
 * Dumps the entry point table of NE/LE/LX file.
 * Requires parse_ne_entry_table() to be called before.
 */
static void dmp_entry_tab( void )
/*******************************/
{
    struct int_entry_pnt    *find;

    if( Entry_pnts == NULL ) {
        return;
    }
    Wdputslc( "\n" );
    Banner( "Entry Point Table" );
    for( find = Entry_pnts; find != NULL; find = find->next ) {
        dmp_an_ord( find );
        Wdputslc( "\n" );
    }
}

/*
 * Dump the tables
 */
void Dmp_ne_tbls( void )
/**********************/
{
    parse_ne_entry_table( New_exe_off + Os2_head.entry_off, Os2_head.entry_size );
    Banner( "Resident Names Table" );
    dmp_res_nonres_tab( New_exe_off + Os2_head.resident_off );
    dmp_mod_ref_tab( New_exe_off + Os2_head.module_off, Os2_head.modrefs );
    if( Os2_head.import_off != Os2_head.entry_off )
        dmp_import_tab( New_exe_off + Os2_head.import_off );
    dmp_entry_tab();
    Wdputslc( "\n" );
    Banner( "Nonresident Names Table" );
    dmp_res_nonres_tab( Os2_head.nonres_off );
    Dmp_relocs();
}

/*
 * Dump the tables
 */
void Dmp_le_lx_tbls( void )
/*************************/
{
    unsigned_32     size;

    Banner( "Resident Names Table" );
    dmp_res_nonres_tab( New_exe_off + Os2_386_head.resname_off );
    Dmp_fixpage_tab( New_exe_off + Os2_386_head.fixpage_off,
                    Os2_386_head.fixrec_off - Os2_386_head.fixpage_off );
    Dmp_fixrec_tab( New_exe_off + Os2_386_head.fixrec_off );
    size = Os2_386_head.impproc_off - Os2_386_head.impmod_off;
    if( size > 1 ) {
        Wdputslc( "\n" );
        Banner( "Import Module Name Table" );
        dmp_imp_tab( New_exe_off + Os2_386_head.impmod_off, size );
    }
    size = Os2_386_head.fixup_size + Os2_386_head.fixpage_off
                    - Os2_386_head.impproc_off;
    if( size > 1 ) {
        Wdputslc( "\n" );
        Banner( "Import Procedure Name Table" );
        dmp_imp_tab( New_exe_off + Os2_386_head.impproc_off, size );
    }
    dmp_ent_tab( New_exe_off + Os2_386_head.entry_off );
    Wdputslc( "\n" );
    Banner( "Nonresident Names Table" );
    dmp_res_nonres_tab( Os2_386_head.nonres_off );
}

/*
 * Dump NE/LE/LX export table content in .DEF file format.
 * Requires load_exe_headers() to be called before.
 * Optionally, parse_??_entry_table() may be called before
 * to enable entry point resolving.
 */
static void dump_os2_exports_as_def( void )
/******************************/
{
    unsigned_16     string_len;
    char            name[MAX_EXPORT_NAME_LEN];
    unsigned_16     ordinal;
    struct int_entry_pnt    *find;

    while( (string_len = read_res_nonres_nam( name, &ordinal )) != 0 ) {
        Wdputs( "    " );
        Wdputs( name );
        if ((Import_format & IMPORT_INCADDR) || (Import_format & IMPORT_INCORDN)) {
            while( string_len++ < 43 ) {
                Wdputc( ' ' );
            }
        }
        if (Import_format & IMPORT_INCORDN) {
            Wdputs( " @" );
            Putdec( ordinal );
        }
        if (Import_format & IMPORT_INCADDR) {
            find = get_entry_point_by_ordinal( ordinal );
            if ( find != NULL ) {
              Wdputs( " ; " );
              Puthex( find->seg_num, 4 );
              Wdputs( ":" );
              Puthex( find->offset, 8 );
            }
        }
        Wdputslc( "\n" );
    }
}

/*
 * Dump NE/LE/LX export table content in .MAP file format.
 * Requires load_exe_headers() to be called before.
 * Optionally, parse_??_entry_table() may be called before
 * to enable entry point resolving.
 */
static void dump_os2_exports_as_map( void )
/******************************/
{
    unsigned_8      string_len;
    char            name[MAX_EXPORT_NAME_LEN];
    unsigned_16     ordinal;
    unsigned_16     seg_num;
    unsigned_32     offset;
    struct int_entry_pnt    *find;

    while( (string_len = read_res_nonres_nam( name, &ordinal )) != 0 ) {
        Wdputs( " " );
        seg_num = 0xFFFFu;
        offset = 0;
        if (Import_format & IMPORT_INCADDR) {
            find = get_entry_point_by_ordinal( ordinal );
            if ( find != NULL ) {
              seg_num = find->seg_num;
              offset = find->offset;
            }
        }
        Puthex( seg_num, 4 );
        Wdputs( ":" );
        Puthex( offset, 8 );
        Wdputs( "       " );
        string_len += 20;
        Wdputs( name );
        if (Import_format & IMPORT_INCORDN) {
            while( string_len++ < 63 ) {
                Wdputc( ' ' );
            }
            Wdputs( "; @" );
            Putdec( ordinal );
        }
        Wdputslc( "\n" );
    }
}

/*
 * Reads EXE file header. Sets 'Form' to proper FORM_* constant,
 * loads the main file header and sets 'New_exe_off'.
 * Returns one of RET_* constants, RET_OK on success.
 */
unsigned_16 load_exe_headers( void )
{
    Wlseek( 0 );
    /* Check executable format; handle stubless modules */
    Wread( &Dos_head, sizeof( Dos_head ) );
    if( Dos_head.signature == DOS_SIGNATURE ) {
        if( Dos_head.reloc_offset != OS2_EXE_HEADER_FOLLOWS ) {
            return( RET_NONEWHDR );
        }
        Wlseek( OS2_NE_OFFSET );
        Wread( &New_exe_off, sizeof( New_exe_off ) );
    } else {
        New_exe_off = 0;
    }

    /* Read appropriate header */
    Wlseek( New_exe_off );
    Wread( &Os2_386_head, sizeof( Os2_386_head ) );
    switch ( Os2_386_head.signature ) {
      case OS2_SIGNATURE_WORD:
        Form = FORM_NE;
        Wlseek( New_exe_off );
        Wread( &Os2_head, sizeof( Os2_head ) );
        break;
      case PE_SIGNATURE:
      case PL_SIGNATURE:
        Form = FORM_PE;
        Wlseek( New_exe_off );
        Wread( &Pe_head, sizeof( Pe_head ) );
        break;
      case OSF_FLAT_SIGNATURE:
        Form = FORM_LE;
        break;
      case OSF_FLAT_LX_SIGNATURE:
        Form = FORM_LX;
        break;
      default:
        return( RET_BADFMT );
    }
    return( RET_OK );
}

/*
 * Returns the NE/LE/LX file resident names table offset.
 * On error, returns 0.
 */
unsigned_32 get_res_nam_tab_offset( void )
{
    switch (Form)
    {
      case FORM_NE:
        return New_exe_off + Os2_head.resident_off;
      case FORM_LE:
      case FORM_LX:
        return New_exe_off + Os2_386_head.resname_off;
      default:
        return 0;
    }
}

/*
 * Returns the NE/LE/LX file non-resident names table offset.
 * On error, returns 0.
 */
unsigned_32 get_nonres_nam_tab_offset( void )
{
    switch (Form)
    {
      case FORM_NE:
        return Os2_head.nonres_off;
      case FORM_LE:
      case FORM_LX:
        return Os2_386_head.nonres_off;
      default:
        return 0;
    }
}

/*
 * Dump the NE/LE/LX exports table in .DEF file format.
 * Assumes that load_exe_headers() was called before.
 */
bool dmp_os2_exports_as_def( void )
/**************************/
{
    unsigned_32     res_nam_tab;
    char            name[MAX_EXPORT_NAME_LEN];
    unsigned_16     ordinal;

    res_nam_tab = get_res_nam_tab_offset();
    if( res_nam_tab == 0 ) return( 0 );

    /* Read and print module name */
    Wlseek( res_nam_tab );
    if( read_res_nonres_nam( name, &ordinal ) == 0 ) {
        return( 0 );
    }
    Wdputs( "LIBRARY " );
    Wdputs( name );
    Wdputslc( "\n" );
    Wdputslc( "EXPORTS\n" );

    /* Print exports in resident table */
    dump_os2_exports_as_def();

    /* Seek to non-resident table */
    res_nam_tab = get_nonres_nam_tab_offset();
    if( res_nam_tab == 0 ) return( 1 );
    Wlseek( res_nam_tab );

    /* See if there is comment */
    if( read_res_nonres_nam( name, &ordinal ) == 0 ) {
        return( 1 );
    }
    if( ordinal != 0 ) {
        Wlseek( res_nam_tab );  /* No comment, seek back */
    }

    /* Print exports in non-resident table */
    dump_os2_exports_as_map();

    return( 1 );
}

/*
 * Dump the NE/LE/LX exports table in .MAP file format.
 * Assumes that load_exe_headers() was called before.
 */
bool dmp_os2_exports_as_map( void )
/**************************/
{
    unsigned_32     res_nam_tab;
    char            name[MAX_EXPORT_NAME_LEN];
    unsigned_16     ordinal;

    res_nam_tab = get_res_nam_tab_offset();
    if( res_nam_tab == 0 ) return( 0 );

    /* Read and print module name */
    Wlseek( res_nam_tab );
    if( read_res_nonres_nam( name, &ordinal ) == 0 ) {
        return( 0 );
    }
    Wdputs( "MAP file for module '" );
    Wdputs( name );
    Wdputslc( "'\n" );
    // Publics in DLLs are usually sorted by name
    Wdputslc( "     Address         Publics by Name\n");

    /* Print exports in resident table */
    dump_os2_exports_as_map();

    /* Seek to non-resident table */
    res_nam_tab = get_nonres_nam_tab_offset();
    if( res_nam_tab == 0 ) return( 1 );
    Wlseek( res_nam_tab );

    /* See if there is comment */
    if( read_res_nonres_nam( name, &ordinal ) == 0 ) {
        return( 1 );
    }
    if( ordinal != 0 ) {
        Wlseek( res_nam_tab );  /* No comment, seek back */
    }

    /* Print exports in non-resident table */
    dump_os2_exports_as_def();

    return( 1 );
}

bool dmp_pe_exports_as_def( void );
bool dmp_pe_exports_as_map( void );

/*
 * Dump exports table of any DLL into .DEF file format.
 */
bool Dmp_dll_exports_as_def( void )
/**************************/
{
    if (load_exe_headers() != RET_OK) return( 0 );
    switch (Form)
    {
      case FORM_NE:
        if (Import_format & IMPORT_INCADDR)
          parse_ne_entry_table( New_exe_off + Os2_head.entry_off, Os2_head.entry_size );
        return dmp_os2_exports_as_def();
      case FORM_LE:
      case FORM_LX:
        return dmp_os2_exports_as_def();
      case FORM_PE:
      case FORM_PL:
        Exp_off = get_pe_section_offset( PE_TBL_EXPORT );
        if( Exp_off == 0 ) return( 0 );
        return dmp_pe_exports_as_def();
      default:
        return 0;
    }
}

/*
 * Dump exports table of any DLL into .MAP file format.
 */
bool Dmp_dll_exports_as_map( void )
/**************************/
{
    if (load_exe_headers() != RET_OK) return( 0 );
    switch (Form)
    {
      case FORM_NE:
        if (Import_format & IMPORT_INCADDR)
          parse_ne_entry_table( New_exe_off + Os2_head.entry_off, Os2_head.entry_size );
        return dmp_os2_exports_as_map();
      case FORM_LE:
      case FORM_LX:
        return dmp_os2_exports_as_map();
      case FORM_PE:
      case FORM_PL:
        Exp_off = get_pe_section_offset( PE_TBL_EXPORT );
        if( Exp_off == 0 ) return( 0 );
        parse_pe_sections();
        return dmp_pe_exports_as_map();
      default:
        return 0;
    }
}
