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
* Description:  Portable Executable tables dump routines.
*
****************************************************************************/


#include <stdio.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#include "wdglb.h"
#include "wdfunc.h"

static  char    *pe_export_msg[] = {
    "4          export flags                      = ",
    "4          time/date stamp                   = ",
    "2          major version number              = ",
    "2          minor version number              = ",
    "4          rva of the Dll asciiz Name        = ",
    "4          first valid exported ordinal      = ",
    "4          no. of entries in Export Addr Tbl = ",
    "4          no. of entries in Name Ptr Tbl    = ",
    "4          rva of the Export Addr Tbl        = ",
    "4          rva of the Export Name Tbl Ptrs   = ",
    "4          rva of Export Ordinal Tbl Entry   = ",
    NULL
};

static  char    *pe_import_msg[] = {
    "4          rva of the start of import lookup tbl = ",
    "4          time/date stamp                       = ",
    "2          major version number                  = ",
    "2          minor version number                  = ",
    "4          rva of the Dll asciiz Name            = ",
    "4          rva of the start of import addresses  = ",
    NULL
};

extern char  Fname[ _MAX_FNAME ];

extern pe_object * Section_objs;
extern unsigned int Section_objs_count;

struct  int_export_itm     *Export_itms = NULL;

/*
 * Allocate a new int_export_itm structure and add it to Export_itms list.
 */
struct int_export_itm *new_export_itm( void )
/**********************************************/
{
    struct int_export_itm    *new_itm;

    new_itm = Wmalloc( sizeof( struct int_export_itm ) );
    new_itm->next = Export_itms;
    Export_itms = new_itm;
    return( new_itm );
}

/*
 * Free the whole Export_itms structure.
 */
void free_export_itms( void )
/**********************************************/
{
    struct int_export_itm    *cr_itm;
    struct int_export_itm    *nx_itm;

    cr_itm = Export_itms;
    Export_itms = NULL;
    while (cr_itm != NULL) {
        nx_itm = cr_itm->next;
        free ( cr_itm );
        cr_itm = nx_itm;
    }
}

/*
 * Parses the Export Section of PE/PL file. Is compatible ONLY with PE/PL files.
 * Stores the data in Export_itms structure.
 */
static void parse_pe_export_section( unsigned_32 addr_offs, unsigned_32 nam_offs,
   unsigned_32 ord_offs, unsigned_32 addr_len, unsigned_32 nam_len, unsigned_32 ord_base )
/*********************************************************************/
{
    unsigned_32     *address;
    unsigned_16     *ordinal;
    unsigned_32     *textname;
    struct int_export_itm    *new_itm;
    unsigned_32     i;

    if( addr_len == 0 ) {
        return;
    }
    /* reading addresses */
    Wlseek( addr_offs );
    i = addr_len * sizeof( unsigned_32 );
    address = Wmalloc( i );
    Wread( address, i );
    /* reading ordinals */
    Wlseek( ord_offs );
    i = addr_len * sizeof( unsigned_16 );
    ordinal = Wmalloc( i );
    Wread( ordinal, i );
    /* reading name offsets */
    if (nam_len > addr_len) nam_len = addr_len;
    Wlseek( nam_offs );
    i = addr_len * sizeof( unsigned_32 );
    textname = Wmalloc( i );
    i = nam_len * sizeof( unsigned_32 );
    Wread( textname, i );
    for (i = nam_len; i < addr_len; i++ )
      textname[i] = 0;

    /* creating Export_itms structure */
    for( i = addr_len; i > 0; i-- )
    {
        new_itm = new_export_itm();
        new_itm->ordinal = ordinal[i-1]+ord_base;
        new_itm->nam_offs = textname[i-1];
        new_itm->body_rva = address[i-1];
        // The following parameters can't be read directly from export table
        new_itm->seg_num = 0xFFFFu;
        new_itm->offset = 0;
    }
    free(address);
    free(ordinal);
    free(textname);
}

/*
 * Computes segments and addresses in Export Section using RVAs.
 * Updates the data in Export_itms structure.
 * Requires Section_objs to be loaded.
 */
void compute_pe_export_section_adresses( void )
/*********************************************************************/
{
    struct int_export_itm  *find;
    unsigned_16 i;
    
    for( find = Export_itms; find != NULL; find = find->next ) {
        i = get_section_idx_for_rva( find->body_rva );
        if (i != 0xFFFFu) {
            find->seg_num = i+1;
            find->offset = find->body_rva - Section_objs[i].rva;
        }
    }
}

/*
 * Dump the Export address Table.
 */
static void dmp_exp_addr( unsigned_32 offset, unsigned_32 num_ent )
/*****************************************************************/
{
    unsigned_32     *address;
    unsigned_32     addr_size;
    unsigned_32     i;

    Wlseek( offset );
    addr_size = num_ent * sizeof( unsigned_32 );
    address = Wmalloc( addr_size );
    Wread( address, addr_size );
    Wdputslc( "\n" );
    Wdputslc( "Export Address Table\n" );
    Wdputslc( "====================\n" );
    for( i = 0; i < num_ent; i++ ) {
        Putdecl( i, 3 );
        Wdputc( ':' );
        Puthex( address[i], 8 );
        if( (i+1) % 4 == 0 ) {
            Wdputslc( "\n" );
        } else {
            Wdputs( "     " );
        }
    }
}

/*
 * Dump the Export Name and Ordinal Tables.
 */
static void dmp_exp_ord_name( unsigned_32 nam_off, unsigned_32 ord_off,
                       unsigned_32 num_ptr, unsigned_32 base )
/*********************************************************************/
{
    unsigned_16     *ord_addr;
    unsigned_32     *nam_addr;
    unsigned_32     addr_size;
    char            *name;
    unsigned_32     i;

    Wlseek( nam_off );
    addr_size = num_ptr * sizeof( unsigned_32 );
    nam_addr = Wmalloc( addr_size );
    Wread( nam_addr, addr_size );
    Wlseek( ord_off );
    addr_size = num_ptr * sizeof( unsigned_16 );
    ord_addr = Wmalloc( addr_size );
    Wread( ord_addr, addr_size );
    Wdputslc( "\n" );
    Wdputslc( "\n" );
    Wdputslc( "  ordinal     name ptr        name\n" );
    Wdputslc( "  =======     ========        ====\n" );
    name = Wmalloc( BUFFERSIZE );
    for( i = 0; i < num_ptr; i++ ) {
        Putdecl( ord_addr[i] + base, 6 );
        Wdputs( "        " );
        Puthex( nam_addr[i], 8 );
        Wdputs( "        " );
        Wlseek( nam_addr[i] - Pe_head.table[ PE_TBL_EXPORT ].rva + Exp_off );
        Wread( name, BUFFERSIZE );
        Wdputs( name );
        Wdputslc( "\n" );
    }
    free(name);
    free(ord_addr);
    free(nam_addr);
}

/*
 * Dump the Import address Table.
 */
static void dmp_imp_addr( unsigned_32 offset )
/********************************************/
{
    unsigned_32     address;
    unsigned_32     addr_size;
    int             i;

    Wlseek( offset );
    Wdputslc( "\n" );
    Wdputslc( "Import Address Table\n" );
    Wdputslc( "====================\n" );
    addr_size = sizeof( unsigned_32 );
    Wread( &address, addr_size );
    for( i = 0; address != 0; i++ ) {
        if( i != 0 ) {
            if( (i) % 4 == 0 ) {
                Wdputslc( "\n" );
            } else {
                Wdputs( "     " );
            }
        }
        Putdecl( i, 3 );
        Wdputc( ':' );
        Puthex( address, 8 );
        Wread( &address, addr_size );
    }
    Wdputslc( "\n" );
    Wdputslc( "\n" );
}

typedef struct {
    unsigned_16         hint;
    char                name[ BUFFERSIZE ];
} pe_hint_name_ent;

/*
 * Dump the Import lookup Table.
 */
static void dmp_imp_lookup( unsigned_32 offset )
/**********************************************/
{
    unsigned_32             address;
    unsigned_32             addr_size;
    int                     i;
    pe_hint_name_ent        hint_name;

    Wlseek( offset );
    Wdputslc( "\n" );
    Wdputslc( "Import Lookup Table\n" );
    Wdputslc( "===================\n" );
    Wdputslc( "       import       hint       name/ordinal\n" );
    Wdputslc( "       ======       ====       ============\n" );

    addr_size = sizeof( unsigned_32 );
    Wread( &address, addr_size );
    for( i = 0; address != 0; ++i ) {
        Wdputs( "       " );
        Puthex( address, 8 );
        if( address & PE_IMPORT_BY_ORDINAL ) {
            Wdputs( "          " );
	    Putdecl( address & ~PE_IMPORT_BY_ORDINAL, 8 );
        } else {
            Wlseek( address - Pe_head.table[ PE_TBL_IMPORT ].rva + Imp_off );
            Wread( &hint_name, sizeof( pe_hint_name_ent ) );
            Putdecl( hint_name.hint, 8 );
            Wdputs( "        " );
            Wdputs( hint_name.name );
        }
        Wdputslc( "\n" );
        offset += sizeof( unsigned_32 );
        Wlseek( offset );
        Wread( &address, addr_size );
    }
}

/*
 * Dump the Export Name and Ordinal Tables.
 */
static void dmp_ord_name( unsigned_32 nam_off, unsigned_32 ord_off,
                   unsigned_32 num_ptr, unsigned_32 base )
/*****************************************************************/
{
    unsigned_16     *ord_addr;
    unsigned_32     *nam_addr;
    unsigned_32     addr_size;
    char            *name;
    int             i;

    Wlseek( nam_off );
    addr_size = num_ptr * sizeof( unsigned_32 );
    nam_addr = Wmalloc( addr_size );
    Wread( nam_addr, addr_size );
    Wlseek( ord_off );
    addr_size = num_ptr * sizeof( unsigned_16 );
    ord_addr = Wmalloc( addr_size );
    Wread( ord_addr, addr_size );
    Wdputslc( "\n" );
    name = Wmalloc( BUFFERSIZE );
    for( i = 0; i < num_ptr; i++ ) {
        Wlseek( nam_addr[i] - Pe_head.table[ PE_TBL_EXPORT ].rva + Exp_off );
        Wread( name, BUFFERSIZE );
        Wdputs( name );
        Wdputc( '.' );
        Wdputc( '\'' );
        Wdputs( Fname );
        Wdputs( ".DLL\'." );
        Putdec( ord_addr[i] + base );
        Wdputslc( "\n" );
    }
    free(name);
    free(ord_addr);
    free(nam_addr);
}

/*
 * Dump the Export Table.
 */
void Dmp_exp_tab( void )
/**********************/
{
    pe_export_directory     pe_export;

    strupr( Fname );
    Wlseek( Exp_off );
    Wread( &pe_export, sizeof( pe_export_directory ) );
    dmp_ord_name( pe_export.name_ptr_table_rva -
            Pe_head.table[ PE_TBL_EXPORT ].rva + Exp_off,
            pe_export.ordinal_table_rva -
            Pe_head.table[ PE_TBL_EXPORT ].rva + Exp_off,
            pe_export.num_name_ptrs, pe_export.ordinal_base );
}

/*
 * Dump the PE/PL Export Table in .DEF file format.
 * Assumes that load_exe_headers() was called before,
 * and Exp_off is set to proper value.
 */
bool dmp_pe_exports_as_def( void )
/**********************/
{
    pe_export_directory     pe_export;
    struct int_export_itm  *find;
    unsigned_16     string_len;
    char            *name;

    Wlseek( Exp_off );
    Wread( &pe_export, sizeof( pe_export_directory ) );
    parse_pe_export_section ( pe_export.address_table_rva -
            Pe_head.table[ PE_TBL_EXPORT ].rva + Exp_off,
            pe_export.name_ptr_table_rva -
            Pe_head.table[ PE_TBL_EXPORT ].rva + Exp_off,
            pe_export.ordinal_table_rva -
            Pe_head.table[ PE_TBL_EXPORT ].rva + Exp_off,
            pe_export.num_eat_entries, pe_export.num_name_ptrs,
            pe_export.ordinal_base );
    if( Export_itms == NULL ) {
        return 0;
    }
    name = Wmalloc( BUFFERSIZE );
    Wdputs( "LIBRARY " );
    Wdputs( Fname );
    Wdputslc( "\n" );
    Wdputslc( "EXPORTS\n" );
    for( find = Export_itms; find != NULL; find = find->next ) {
        Wlseek( find->nam_offs - Pe_head.table[ PE_TBL_EXPORT ].rva + Exp_off );
        Wread( name, BUFFERSIZE-1 );
        name[BUFFERSIZE-1] = '\0';
        Wdputs( "    " );
        Wdputs( name );
        string_len = strlen( name );
        if ((Import_format & IMPORT_INCADDR) || (Import_format & IMPORT_INCORDN)) {
            while( string_len++ < 43 ) {
                Wdputc( ' ' );
            }
        }
        if (Import_format & IMPORT_INCORDN) {
            Wdputs( " @" );
            Putdec( find->ordinal );
        }
        if (Import_format & IMPORT_INCADDR) {
              Wdputs( " ; RVA=0x" );
              Puthex( find->body_rva, 8 );
        }
        Wdputslc( "\n" );
    }
    free_export_itms();
    return ( 1 );
}

/*
 * Dump the PE/PL Export Table in .MAP file format.
 * Assumes that load_exe_headers() was called before,
 * and Exp_off is set to proper value.
 */
bool dmp_pe_exports_as_map( void )
/**********************/
{
    pe_export_directory     pe_export;
    struct int_export_itm  *find;
    unsigned_16     string_len;
    char            *name;

    Wlseek( Exp_off );
    Wread( &pe_export, sizeof( pe_export_directory ) );
    parse_pe_export_section ( pe_export.address_table_rva -
            Pe_head.table[ PE_TBL_EXPORT ].rva + Exp_off,
            pe_export.name_ptr_table_rva -
            Pe_head.table[ PE_TBL_EXPORT ].rva + Exp_off,
            pe_export.ordinal_table_rva -
            Pe_head.table[ PE_TBL_EXPORT ].rva + Exp_off,
            pe_export.num_eat_entries, pe_export.num_name_ptrs,
            pe_export.ordinal_base );
    if( Export_itms == NULL ) {
        return 0;
    }
    compute_pe_export_section_adresses();
    name = Wmalloc( BUFFERSIZE );
    Wdputs( "MAP file for module '" );
    Wdputs( Fname );
    Wdputslc( "'\n" );
    // Publics in PEs are sorted by name
    Wdputslc( "     Address         Publics by Name\n");
    for( find = Export_itms; find != NULL; find = find->next ) {
        string_len = 0;
        Wdputs( " " );
        Puthex( find->seg_num, 4 );
        Wdputs( ":" );
        Puthex( find->offset, 8 );
        Wdputs( "       " );
        string_len += 20;
        Wlseek( find->nam_offs - Pe_head.table[ PE_TBL_EXPORT ].rva + Exp_off );
        Wread( name, BUFFERSIZE-1 );
        name[BUFFERSIZE-1] = '\0';
        Wdputs( name );
        string_len += strlen( name );
        if (Import_format & IMPORT_INCORDN) {
            while( string_len++ < 63 ) {
                Wdputc( ' ' );
            }
            Wdputs( "; @" );
            Putdec( find->ordinal );
        }
        Wdputslc( "\n" );
    }
    free ( name );
    free_export_itms();
    return ( 1 );
}

/*
 * Dump the Export Table.
 */
void Dmp_exports( void )
/**********************/
{
    pe_export_directory     pe_export;

    Wlseek( Exp_off );
    Wread( &pe_export, sizeof( pe_export_directory ) );
    Banner( "Export Directory Table" );
    Dump_header( (char *)&pe_export.flags, pe_export_msg );
    dmp_exp_addr( pe_export.address_table_rva -
            Pe_head.table[ PE_TBL_EXPORT ].rva + Exp_off,
            pe_export.num_eat_entries );
    dmp_exp_ord_name( pe_export.name_ptr_table_rva -
            Pe_head.table[ PE_TBL_EXPORT ].rva + Exp_off,
            pe_export.ordinal_table_rva -
            Pe_head.table[ PE_TBL_EXPORT ].rva + Exp_off,
            pe_export.num_name_ptrs, pe_export.ordinal_base );
    Wdputslc( "\n" );
}

/*
 * Dump the Import Table.
 */
void Dmp_imports( void )
/**********************/
{
    pe_import_directory     pe_import;
    unsigned_32             offset;
    char                    name[BUFFERSIZE];

    offset = Imp_off;
    for( ;; ) {
        Wlseek( offset );
        Wread( &pe_import, sizeof( pe_import_directory ) );
        if( pe_import.import_lookup_table_rva == 0 ) break;
        Banner( "Import Directory Table" );
        Dump_header( (char *)&pe_import.import_lookup_table_rva, pe_import_msg );
        Wlseek( pe_import.name_rva - Pe_head.table[ PE_TBL_IMPORT ].rva + Imp_off );
        Wread( name, sizeof( name ) );
        Wdputs( "          DLL name = <" );
        Wdputs( name );
        Wdputslc( ">\n" );
        dmp_imp_lookup( pe_import.import_lookup_table_rva -
                Pe_head.table[ PE_TBL_IMPORT ].rva + Imp_off );
        dmp_imp_addr( pe_import.import_address_table_rva -
                Pe_head.table[ PE_TBL_IMPORT ].rva + Imp_off );
        offset += sizeof( pe_import_directory );
    }
}
