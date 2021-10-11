/****************************************************************************
* WARNING: simplified version, dumb cut to allow WDUMP compilation in GCC
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
* Description:  DOS and DPMI interrupt interfacing.
*
****************************************************************************/


#ifndef _TINYIO_H_INCLUDED

#if defined(__SW_ZDP) && !defined(ZDP)
#define ZDP
#endif

#include <errno.h>
#include "watcom.h"

#pragma pack( 1 )

/*
 * miscellaneous definitions
 */

/*
  open access mask

  bits  use
  ====  ===
  7     inheritance flag
  4-6   sharing mode
  3     reserved (=0)
  0-2   read/write access
*/
typedef enum {
    TIO_READ                    = 0x00,
    TIO_WRITE                   = 0x01,
    TIO_READ_WRITE              = 0x02,
    TIO_DENY_COMPATIBILITY      = 0x00,
    TIO_DENY_READ_WRITE         = 0x10,
    TIO_DENY_WRITE              = 0x20,
    TIO_DENY_READ               = 0x30,
    TIO_DENY_NONE               = 0x40,
    TIO_INHERITANCE             = 0x80,
    TIO_READ_DENY_WRITE         = TIO_READ | TIO_DENY_WRITE,
    TIO_NULL_ATTR               = 0x00
} open_attr;

typedef enum {
    TIO_SEEK_START              = 0,
    TIO_SEEK_SET                = 0,
    TIO_SEEK_CURR               = 1,
    TIO_SEEK_CUR                = 1,
    TIO_SEEK_END                = 2,
} seek_info;

typedef enum {
    TIO_STDIN_FILENO            = 0,
    TIO_STDOUT_FILENO           = 1,
    TIO_STDERR_FILENO           = 2,
    TIO_STDAUX_FILENO           = 3,
    TIO_STDPRN_FILENO           = 4
} tio_file_handles;

typedef enum {
    TIO_NORMAL              = 0x00,
    TIO_READ_ONLY           = 0x01,
    TIO_HIDDEN              = 0x02,
    TIO_SYSTEM              = 0x04,
    TIO_VOLUME_LABEL        = 0x08,
    TIO_SUBDIRECTORY        = 0x10,
    TIO_ARCHIVE             = 0x20,
} create_attr;

typedef enum {
    TIO_CREATE              = 0x01, /* from pg 3-112 of The Programmers */
    TIO_OPEN                = 0x10, /* PC Source Book */
    TIO_TRUNCATE            = 0x20,
} create_action;

#define TINY_IN     0
#define TINY_OUT    1
#define TINY_ERR    2

/*
 * return value from TinyGetDeviceInfo
 */
enum {
    TIO_CTL_CONSOLE_IN      = 0x0001,
    TIO_CTL_CONSOLE_OUT     = 0x0002,
    TIO_CTL_NULL            = 0x0004,
    TIO_CTL_CLOCK           = 0x0008,
    TIO_CTL_SPECIAL         = 0x0010, /* won't be supported in the future */
    TIO_CTL_RAW             = 0x0020,
    TIO_CTL_EOF             = 0x0040,
    TIO_CTL_DEVICE          = 0x0080,

    TIO_CTL_DISK_DRIVE_MASK = 0x001f, /* if ret & CTL_DEVICE == 0 */
};

/*
 * return values from calls (same names as OS/2 ERROR_* macros)
 */
enum {
    TIO_INVALID_FUNCTION    = 1,
    TIO_FILE_NOT_FOUND,
    TIO_PATH_NOT_FOUND,
    TIO_TOO_MANY_OPEN_FILES,
    TIO_ACCESS_DENIED,
    TIO_INVALID_HANDLE,
    TIO_ARENA_TRASHED,
    TIO_NOT_ENOUGH_MEMORY,
    TIO_INVALID_BLOCK,
    TIO_BAD_ENVIRONMENT,
    TIO_BAD_FORMAT,
    TIO_INVALID_ACCESS,
    TIO_INVALID_DATA,

    TIO_INVALID_DRIVE       = 15,
    TIO_CURRENT_DIRECTORY,
    TIO_NOT_SAME_DEVICE,
    TIO_NO_MORE_FILES,
    TIO_WRITE_PROTECT,
    TIO_BAD_UNIT,
    TIO_NOT_READY,
    TIO_BAD_COMMAND,
    TIO_CRC,
    TIO_BAD_LENGTH,
    TIO_SEEK,
    TIO_NOT_DOS_DISK,
    TIO_SECTOR_NOT_FOUND,
    TIO_OUT_OF_PAPER,
    TIO_WRITE_FAULT,
    TIO_READ_FAULT,
    TIO_GEN_FAILURE,
    TIO_SHARING_VIOLATION,
    TIO_LOCK_VIOLATION,
    TIO_WRONG_DISK,
    TIO_FCB_UNAVAILABLE,

    TIO_FILE_EXISTS         = 80,

    TIO_CANNOT_MAKE         = 82,
    TIO_FAIL_I24,

    TIO_FIND_ERROR          = TIO_FILE_NOT_FOUND,
    TIO_FIND_NO_MORE_FILES  = TIO_NO_MORE_FILES,
};

/*
 * stuff for TinyGetFileStamp & TinySetFileStamp
 */
#ifdef _OLD_TINYIO_

typedef struct {
    uint_16     hms;
    uint_16     ymd;
} tiny_file_stamp_t;

#define HMS_pack( hr, min, sec )        ((hr)*2048+(min)*32+(sec)/2)
#define YMD_pack( yr, mon, day )        (((yr)-1980)*512+(mon)*32+(day))

#else

typedef struct {
    uint_16             twosecs : 5;    /* seconds / 2 */
    uint_16             minutes : 6;
    uint_16             hours   : 5;
} tiny_ftime_t;

typedef struct {
    uint_16             day     : 5;
    uint_16             month   : 4;
    uint_16             year    : 7;
} tiny_fdate_t;

typedef struct {
    tiny_ftime_t        time;
    tiny_fdate_t        date;
} tiny_file_stamp_t;

#endif

/*
 * format of DTA for TinyFindFirst/Next
 */
#define TIO_NAME_MAX    13  /* filename.ext\0 */
typedef struct {
    char                reserved[ 21 ]; /* dos uses this area */
    uint_8              attr;           /* attribute of file */
    tiny_ftime_t        time;
    tiny_fdate_t        date;
    uint_32             size;
    char                name[ TIO_NAME_MAX ];
} tiny_find_t;

/*
 * return from TinyGetDate and TinyGetTime
 */
typedef struct tiny_date_t {
    uint_8              day_of_month;   /* 1 - 31 */
    uint_8              month;          /* 1 - 12 */
    uint_8              year;           /* year minus 1900 */
    uint_8              day_of_week;    /* 0 - Sun, ..., 6 - Sat */
} tiny_date_t;

typedef struct tiny_time_t {
    uint_8              hundredths;     /* 0 - 99 */
    uint_8              seconds;        /* 0 - 59 */
    uint_8              minutes;        /* 0 - 59 */
    uint_8              hour;           /* 0 - 23 */
} tiny_time_t;

/*
 * return from TinyGetDOSVersion
 */
typedef struct tiny_dos_version {
    uint_8              major;
    uint_8              minor;
} tiny_dos_version;

/*
 * return from TinyGetCountry
 */
typedef enum {
    TDATE_M_D_Y         = 0,
    TDATE_D_M_Y         = 1,
    TDATE_Y_M_D         = 2,
} date_format;

typedef enum {
    TTIME_12_HOUR       = 0,
    TTIME_24_HOUR       = 1,
} time_format;

enum {                          /* mask values for 'currency_symbol_position' */
    TPOSN_FOLLOWS_VALUE = 0x01, /* currency symbol follows value */
    TPOSN_ONE_SPACE     = 0x02, /* currency symbol is one space from value */
};

/*
 * call_struct definition for TinyDPMISimulateRealInt
 */
typedef struct {
    uint_32     edi;
    uint_32     esi;
    uint_32     ebp;
    uint_32     reserved;
    uint_32     ebx;
    uint_32     edx;
    uint_32     ecx;
    uint_32     eax;
    uint_16     flags;
    uint_16     es;
    uint_16     ds;
    uint_16     fs;
    uint_16     gs;
    uint_16     ip;
    uint_16     cs;
    uint_16     sp;
    uint_16     ss;
} call_struct;

/* Definitions for manipulating protected mode descriptors ... used
 * with TinyDPMIGetDescriptor, TinyDPMISetDescriptor, TinyDPMISetRights, etc.
 */
typedef enum {
    TIO_ACCESSED        = 0x01,
    TIO_RDWR            = 0x02,
    TIO_EXPAND_DOWN     = 0x04,
    TIO_EXECUTE         = 0x08,
    TIO_MUST_BE_1       = 0x10,
    TIO_PRESENT         = 0x80,
    TIO_USER_AVAIL      = 0x1000,
    TIO_MUST_BE_0       = 0x2000,
    TIO_USE32           = 0x4000,
    TIO_PAGE_GRANULAR   = 0x8000
} tiny_dscp_flags;

typedef struct {
    uint_8      accessed : 1;
    uint_8      rdwr     : 1;
    uint_8      exp_down : 1;
    uint_8      execute  : 1;
    uint_8      mustbe_1 : 1;
    uint_8      dpl      : 2;
    uint_8      present  : 1;
} tiny_dscp_type;

typedef struct {
    uint_8               : 4;
    uint_8      useravail: 1;
    uint_8      mustbe_0 : 1;
    uint_8      use32    : 1;
    uint_8      page_gran: 1;
} tiny_dscp_xtype;

typedef struct {
    uint_16             lim_0_15;
    uint_16             base_0_15;
    uint_8              base_16_23;
    tiny_dscp_type      type;
    union {
        struct {
            uint_8      lim_16_19: 4;
            uint_8               : 4;
        };
        tiny_dscp_xtype xtype;
    };
    uint_8              base_24_31;
} tiny_dscp;

/*
 *  DOS FCB structure definitions for TinyFCB... functions
 */
#define TIO_EXTFCB_FLAG    0xff
typedef union {
    uint_8  extended_fcb_flag;  /* == TIO_EXTFCB_FLAG when extended */

    /* from MSDOS Encyclopedia pg 1473 */
    struct {
        uint_8          drive_identifier;   /* != TIO_EXTFCB_FLAG */
        char            filename[ 8 ];
        char            file_extension[ 3 ];
        uint_16         current_block_num;
        uint_16         record_size;
        uint_32         file_size;
        tiny_fdate_t    date_stamp;
        tiny_ftime_t    time_stamp;
        char            reserved[ 8 ];
        uint_8          current_record_num;
        uint_8          random_record_number[ 4 ];
    } normal;

    /* from MSDOS Encyclopedia pg 1476 */
    struct {
        uint_8          extended_fcb_flag;  /* == TIO_EXTFCB_FLAG */
        char            reserved1[ 6 ];
        create_attr     file_attribute;
        uint_8          drive_identifier;
        char            filename[ 8 ];
        char            file_extension[ 3 ];
        uint_16         current_block_num;
        uint_16         record_size;
        uint_32         file_size;
        tiny_fdate_t    date_stamp;
        tiny_ftime_t    time_stamp;
        char            reserved2[ 9 ];
        uint_8          current_record_num;
        uint_8          random_record_number[ 4 ];
    } extended;
} tiny_fcb_t;

enum {
    TIO_PRSFN_IGN_SEPARATORS    = 0x01, /* if (separators) present ignore them*/
    TIO_PRSFN_DONT_OVW_DRIVE    = 0x02, /* leave drive in FCB unaltered if not*/
    TIO_PRSFN_DONT_OVW_FNAME    = 0x04, /* present in parsed string.  Same for*/
    TIO_PRSFN_DONT_OVW_EXT      = 0x08  /* fname and ext                      */
};

/*
 * type definitions
 */
typedef int             tiny_handle_t;
typedef int_32          tiny_ret_t;

/*
 * macro defintions
 */
#define TINY_ERROR( h )         ((int_32)(h)<0)
#define TINY_OK( h )            ((int_32)(h)>=0)
#define TINY_INFO( h )          ((uint_16)(h))
#define TINY_LINFO( h )          ((uint_32)(h))
/* 5-nov-90 AFS TinySeek returns a 31-bit offset that must be sign extended */
#define TINY_INFO_SEEK( h )     (((int_32)(h)^0xc0000000L)-0xc0000000L)


#pragma pack()
#define _TINYIO_H_INCLUDED
#endif
