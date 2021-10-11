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
* Description:  Implementation of non-standard functions used by bootstrap
*
****************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <ctype.h>

#if defined(__UNIX__)
    #include <dirent.h>
    #include <sys/stat.h>
    #include <fcntl.h>
#if defined(__QNX__)
    #include <sys/io_msg.h>
#endif
#else
    #include <direct.h>
    #if defined(__OS2__)
        #include <wos2.h>
    #elif defined(__NT__)
        #include <windows.h>
    #endif
#endif

#include "clibext.h"
#include "wreslang.h"
#include "tinyio_types.h"

#define __set_errno( err ) errno = (err)

char **_argv;
int  _argc;

/****************************************************************************
*
* Description:  Shared alphabet array for conversion of integers to ASCII.
*
****************************************************************************/

static const char __Alphabet[] = "0123456789abcdefghijklmnopqrstuvwxyz";


char *utoa( unsigned value, char *buffer, int radix )
{
    char   *p = buffer;
    char        *q;
    unsigned    rem;
    unsigned    quot;
    char        buf[34];    // only holds ASCII so 'char' is OK

    buf[0] = '\0';
    q = &buf[1];
    do {
        rem = value % radix;
        quot = value / radix;
        *q = __Alphabet[rem];
        ++q;
        value = quot;
    } while( value != 0 );
    while( (*p++ = (char)*--q) )
        ;
    return( buffer );
}


char *itoa( int value, char *buffer, int radix )
{
    char   *p = buffer;

    if( radix == 10 ) {
        if( value < 0 ) {
            *p++ = '-';
            value = - value;
        }
    }
    utoa( value, p, radix );
    return( buffer );
}

/****************************************************************************
*
* Description:  Implementation of ltoa().
*
****************************************************************************/

char *ultoa( unsigned long value, char *buffer, int radix )
{
    char   *p = buffer;
    char        *q;
    unsigned    rem;
    char        buf[34];        // only holds ASCII so 'char' is OK

    buf[0] = '\0';
    q = &buf[1];
    do {
        rem = value % radix;
        value = value / radix;
        *q = __Alphabet[rem];
        ++q;
    } while( value != 0 );
    while( (*p++ = (char)*--q) )
        ;
    return( buffer );
}


char *ltoa( long value, char *buffer, int radix )
{
    char   *p = buffer;

    if( radix == 10 ) {
        if( value < 0 ) {
            *p++ = '-';
            value = - value;
        }
    }
    ultoa( value, p, radix );
    return( buffer );
}

/****************************************************************************
*
* Description:  Platform independent _splitpath() implementation.
*
****************************************************************************/

#if defined(__UNIX__)
  #define PC '/'
#else   /* DOS, OS/2, Windows, Netware */
  #define PC '\\'
  #define ALT_PC '/'
#endif

#ifdef __NETWARE__
  #undef _MAX_PATH
  #undef _MAX_SERVER
  #undef _MAX_VOLUME
  #undef _MAX_DRIVE
  #undef _MAX_DIR
  #undef _MAX_FNAME
  #undef _MAX_EXT
  #undef _MAX_NAME

  #define _MAX_PATH    255 /* maximum length of full pathname */
  #define _MAX_SERVER  48  /* maximum length of server name */
  #define _MAX_VOLUME  16  /* maximum length of volume component */
  #define _MAX_DRIVE   3   /* maximum length of drive component */
  #define _MAX_DIR     255 /* maximum length of path component */
  #define _MAX_FNAME   9   /* maximum length of file name component */
  #define _MAX_EXT     5   /* maximum length of extension component */
  #define _MAX_NAME    13  /* maximum length of file name (with extension) */
#endif


static void copypart( char *buf, const char *p, int len, int maxlen )
{
    if( buf != NULL ) {
        if( len > maxlen ) len = maxlen;
            #ifdef __UNIX__
                memcpy( buf, p, len );
                buf[len] = '\0';
            #else
                len = _mbsnccnt( p, len );          /* # chars in len bytes */
                _mbsncpy( buf, p, len );            /* copy the chars */
                buf[ _mbsnbcnt(buf,len) ] = '\0';
            #endif
    }
}

#if !defined(_MAX_NODE)
#define _MAX_NODE   _MAX_DRIVE  /*  maximum length of node name w/ '\0' */
#endif

/* split full QNX path name into its components */

/* Under QNX we will map drive to node, dir to dir, and
 * filename to (filename and extension)
 *          or (filename) if no extension requested.
 */


/****************************************************************************
*
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/

#if defined(__UNIX__)
  #define PC '/'
#else   /* DOS, OS/2, Windows */
  #define PC '\\'
  #define ALT_PC '/'
#endif

/* split full Unix path name into its components */

/* Under Unix we will map drive to node, dir to dir, and
 * filename to (filename and extension)
 *          or (filename) if no extension requested.
 */


static char *pcopy( char **pdst, char *dst, const char *b_src, const char *e_src ) {
/*========================================================================*/

    unsigned    len;

    if( pdst == NULL ) return( dst );
    *pdst = dst;
    len = e_src - b_src;
    if( len >= _MAX_PATH2 )
    {
        len = _MAX_PATH2 - 1;
    }
        #ifdef __UNIX__
            memcpy( dst, b_src, len );
            dst[len] = '\0';
            return( dst + len + 1 );
        #else
            len = _mbsnccnt( b_src, len );          /* # chars in len bytes */
            _mbsncpy( dst, b_src, len );            /* copy the chars */
            dst[ _mbsnbcnt(dst,len) ] = '\0';
            return( dst + _mbsnbcnt(dst,len) + 1 );
        #endif
}


/****************************************************************************
*
* Description:  Platform independent _makepath() implementation.
*
****************************************************************************/

#undef _makepath

#if defined(__UNIX__)
  #define PC '/'
#else   /* DOS, OS/2, Windows, Netware */
  #define PC '\\'
  #define ALT_PC '/'
#endif


#if defined(__UNIX__)

/* create full Unix style path name from the components */

void _makepath(
        char           *path,
        const char  *node,
        const char  *dir,
        const char  *fname,
        const char  *ext )
{
    *path = '\0';

    if( node != NULL ) {
        if( *node != '\0' ) {
            strcpy( path, node );
            path = strchr( path, '\0' );

            /* if node did not end in '/' then put in a provisional one */
            if( path[-1] == PC )
                path--;
            else
                *path = PC;
        }
    }
    if( dir != NULL ) {
        if( *dir != '\0' ) {
            /*  if dir does not start with a '/' and we had a node then
                    stick in a separator
            */
            if( (*dir != PC) && (*path == PC) ) path++;

            strcpy( path, dir );
            path = strchr( path, '\0' );

            /* if dir did not end in '/' then put in a provisional one */
            if( path[-1] == PC )
                path--;
            else
                *path = PC;
        }
    }

    if( fname != NULL ) {
        if( (*fname != PC) && (*path == PC) ) path++;

        strcpy( path, fname );
        path = strchr( path, '\0' );

    } else {
        if( *path == PC ) path++;
    }
    if( ext != NULL ) {
        if( *ext != '\0' ) {
            if( *ext != '.' )  *path++ = '.';
            strcpy( path, ext );
            path = strchr( path, '\0' );
        }
    }
    *path = '\0';
}

#elif defined( __NETWARE__ )

/*
    For silly two choice DOS path characters / and \,
    we want to return a consistent path character.
*/

static char pickup( char c, char *pc_of_choice )
{
    if( c == PC || c == ALT_PC ) {
        if( *pc_of_choice == '\0' ) *pc_of_choice = c;
        c = *pc_of_choice;
    }
    return( c );
}

extern void _makepath( char *path, const char *volume,
                const char *dir, const char *fname, const char *ext )
{
    char first_pc = '\0';

    if( volume != NULL ) {
        if( *volume != '\0' ) {
            do {
                *path++ = *volume++;
            } while( *volume != '\0' );
            if( path[ -1 ] != ':' ) {
                *path++ = ':';
            }
        }
    }
    *path = '\0';
    if( dir != NULL ) {
        if( *dir != '\0' ) {
            do {
                *path++ = pickup( *dir++, &first_pc );
            } while( *dir != '\0' );
            /* if no path separator was specified then pick a default */
            if( first_pc == '\0' ) first_pc = PC;
            /* if dir did not end in path sep then put in a provisional one */
            if( path[-1] == first_pc ) {
                path--;
            } else {
                *path = first_pc;
            }
        }
    }
    /* if no path separator was specified thus far then pick a default */
    if( first_pc == '\0' ) first_pc = PC;
    if( fname != NULL ) {
        if( (pickup( *fname, &first_pc ) != first_pc)
            && (*path == first_pc) ) path++;
        while( *fname != '\0' ) *path++ = pickup( *fname++, &first_pc );
    } else {
        if( *path == first_pc ) path++;
    }
    if( ext != NULL ) {
        if( *ext != '\0' ) {
            if( *ext != '.' )  *path++ = '.';
            while( *ext != '\0' ) *path++ = *ext++;
        }
    }
    *path = '\0';
}

#else

/*
    For silly two choice DOS path characters / and \,
    we want to return a consistent path character.
*/

static unsigned pickup( unsigned c, unsigned *pc_of_choice )
{
    if( c == PC || c == ALT_PC ) {
        if( *pc_of_choice == '\0' ) *pc_of_choice = c;
        c = *pc_of_choice;
    }
    return( c );
}

#endif

/****************************************************************************
*
* Description:  Implementation of fullpath() - returns fully qualified
*               pathname of a file.
*
****************************************************************************/

#define _WILL_FIT( c )  if(( (c) + 1 ) > size ) {       \
                            __set_errno( ERANGE );      \
                            return( NULL );             \
                        }                               \
                        size -= (c);

#ifdef __UNIX__
#define _IS_SLASH( c )  ((c) == '/')
#else
#define _IS_SLASH( c )  (( (c) == '/' ) || ( (c) == '\\' ))
#endif

#if !defined( __NT__ ) && !defined( __NETWARE__ ) && !defined( __UNIX__ )
#pragma on (check_stack);
#endif

#ifdef __NETWARE__
extern char *ConvertNameToFullPath( const char *, char * );
#endif

#if defined(__QNX__)
static char *__qnx_fullpath(char *fullpath, const char *path)
{
    struct {
            struct _io_open _io_open;
            char  m[_QNX_PATH_MAX];
    } msg;
    int             fd;

    msg._io_open.oflag = _IO_HNDL_INFO;
    fd = __resolve_net( _IO_HANDLE, 1, &msg._io_open, path, 0, fullpath );
    if( fd != -1) {
        close(fd);
    } else if (errno != ENOENT) {
        return 0;
    } else {
        __resolve_net( 0, 0, &msg._io_open, path, 0, fullpath );
    }
    return fullpath;
}
#endif



/****************************************************************************
*
* Description:  Implementation of strlwr(). 
*
****************************************************************************/

 char *strlwr( char *str ) {
    char    *p;
    unsigned char   c;

    p = str;
    while( (c = *p) ) {
        c -= 'A';
        if( c <= 'Z' - 'A' ) {
            c += 'a';
            *p = c;
        }
        ++p;
    }
    return( str );
}
/****************************************************************************
*
* Description:  Implementation of strupr().
*
****************************************************************************/

 char *strupr( char *str ) {
    char    *p;
    unsigned char   c;

    p = str;
    while( (c = *p) ) {
        c -= 'a';
        if( c <= 'z' - 'a' ) {
            c += 'A';
            *p = c;
        }
        ++p;
    }
    return( str );
}

/****************************************************************************
*
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/

int memicmp( const void *in_s1, const void *in_s2, size_t len )
    {
        const unsigned char *   s1 = (const unsigned char *)in_s1;
        const unsigned char *   s2 = (const unsigned char *)in_s2;
        unsigned char           c1;
        unsigned char           c2;

        for( ; len; --len )  {
            c1 = *s1;
            c2 = *s2;
            if( c1 >= 'A'  &&  c1 <= 'Z' )  c1 += 'a' - 'A';
            if( c2 >= 'A'  &&  c2 <= 'Z' )  c2 += 'a' - 'A';
            if( c1 != c2 ) return( c1 - c2 );
            ++s1;
            ++s2;
        }
        return( 0 );    /* both operands are equal */
    }

/****************************************************************************
*
* Description:  Implementation of tell(). 
*
****************************************************************************/

off_t tell( int handle )
{
    return( lseek( handle, 0L, SEEK_CUR ) );
}
/****************************************************************************
*
* Description:  Implementation of _rotl().
*
****************************************************************************/

/****************************************************************************
*
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/

 char *strnset( char *str, int c, size_t len )
    {
        char *p;

        for( p = str; len; --len ) {
            if( *p == '\0' ) break;
            *p++ = c;
        }
        return( str );
    }

/****************************************************************************
*
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/

char *strrev( char *str ) {  /* reverse characters in string */
        char       *p1;
        char       *p2;
        char       c1;
        char       c2;

        p1 = str;
        p2 = p1 + strlen( p1 ) - 1;
        while( p1 < p2 ) {
            c1 = *p1;
            c2 = *p2;
            *p1 = c2;
            *p2 = c1;
            ++p1;
            --p2;
        }
        return( str );
}

/****************************************************************************
*
* Description:  Implements POSIX filelength() function
*
****************************************************************************/

long filelength( int handle )
{
    long                current_posn, file_len;

    current_posn = lseek( handle, 0L, SEEK_CUR );
    if( current_posn == -1L )
    {
        return( -1L );
    }
    file_len = lseek( handle, 0L, SEEK_END );
    lseek( handle, current_posn, SEEK_SET );

    return( file_len );
}

/****************************************************************************
*
* Description:  Implementation of eof().
*
****************************************************************************/

int eof( int handle )         /* determine if at EOF */
{
    off_t   current_posn, file_len;

    file_len = filelength( handle );
    if( file_len == -1L )
        return( -1 );
    current_posn = tell( handle );
    if( current_posn == -1L )
        return( -1 );
    if( current_posn == file_len )
        return( 1 );
    return( 0 );
}

/****************************************************************************
*
* Description:  Implementation of _cmdname().
*
****************************************************************************/

/* NOTE: This file isn't used for QNX. It's got its own version. */

#ifdef __APPLE__

#include <mach-o/dyld.h>

/* No procfs on Darwin, have to use special API */ 

char *_cmdname( char *name )
{
    uint32_t    len = 4096;

    _NSGetExecutablePath( name, &len );
    return( name );
}

#elif defined __UNIX__

char *_cmdname( char *name )
{
    int save_errno = errno;
    int result = readlink( "/proc/self/exe", name, PATH_MAX );
    if( result == -1 ) {
        /* try another way for BSD */
        result = readlink( "/proc/curproc/file", name, PATH_MAX );
    }
    errno = save_errno;

    /* fall back to argv[0] if readlink doesn't work */
    if( result == -1 || result == PATH_MAX )
        return( strcpy( name, _argv[0] ) );

    /* readlink does not add a NUL so we need to do it ourselves */
    name[result] = '\0';
    return( name );
}

#else

char *_cmdname( char *name )
{
    return( strcpy( name, _argv[0] ) );
}

#endif

/****************************************************************************
*
* Description:  Implementation of getcmd() and _bgetcmd() for Unix.
*
****************************************************************************/

int (_bgetcmd)( char *buffer, int len )
{
    int     total;
    int     i;
    char    *word;
    char    *p     = NULL;
    char    **argv = &_argv[1];

    --len; // reserve space for NULL byte

    if( buffer && (len > 0) ) {
        p  = buffer;
        *p = '\0';
    }

    /* create approximation of original command line */
    for( word = *argv++, i = 0, total = 0; word; word = *argv++ ) {
        i      = strlen( word );
        total += i;

        if( p ) {
            if( i >= len ) {
                strncpy( p, word, len );
                p[len] = '\0';
                p      = NULL;
                len    = 0;
            } else {
                strcpy( p, word );
                p   += i;
                len -= i;
            }
        }

        /* account for at least one space separating arguments */
        if( *argv ) {
            if( p ) {
                *p++ = ' ';
                --len;
            }
            ++total;
        }
    }

    return( total );
}


char *(getcmd)( char *buffer )
{
    _bgetcmd( buffer, INT_MAX );
    return( buffer );
}

/****************************************************************************
*
* Description:  This function searches for the specified file in the
*               1) current directory or, failing that,
*               2) the paths listed in the specified environment variable
*               until it finds the first occurrence of the file.
*
****************************************************************************/

#if defined(__UNIX__)
        #define PATH_SEPARATOR '/'
        #define LIST_SEPARATOR ':'
#else
        #define PATH_SEPARATOR '\\'
        #define LIST_SEPARATOR ';'
#endif

void _searchenv( const char *name, const char *env_var, char *buffer )
{
    char        *p, *p2;
    int         prev_errno;
    size_t      len;

    prev_errno = errno;
    if( access( name, F_OK ) == 0 ) {
        p = buffer;                                 /* JBS 90/3/30 */
        len = 0;                                    /* JBS 04/1/06 */
        for( ;; ) {
            if( name[0] == PATH_SEPARATOR ) break;
            if( name[0] == '.' ) break;
#ifndef __UNIX__
            if( name[0] == '/' ) break;
            if( (name[0] != '\0') && (name[1] == ':') ) break;
#endif
            getcwd( buffer, _MAX_PATH );
            len = strlen( buffer );
            p = &buffer[ len ];
            if( p[-1] != PATH_SEPARATOR ) {
                if( len < (_MAX_PATH - 1) ) {
                    *p++ = PATH_SEPARATOR;
                    len++;
                }
            }
            break;
        }
        *p = '\0';
        strncat( p, name, (_MAX_PATH - 1) - len );
        return;
    }
    p = getenv( env_var );
    if( p != NULL ) {
        for( ;; ) {
            if( *p == '\0' ) break;
            p2 = buffer;
            len = 0;                                /* JBS 04/1/06 */
            while( *p ) {
                if( *p == LIST_SEPARATOR ) break;
                if( *p != '"' ) {
                    if( len < (_MAX_PATH-1) ) {
                        *p2++ = *p; /* JBS 00/9/29 */
                        len++;
                    }
                }
                p++;
            }
            /* check for zero-length prefix which represents CWD */
            if( p2 != buffer ) {                    /* JBS 90/3/30 */
                if( p2[-1] != PATH_SEPARATOR
#ifndef __UNIX__
                    &&  p2[-1] != '/'
                    &&  p2[-1] != ':'
#endif
                    ) {
                    if( len < (_MAX_PATH - 1) ) {
                        *p2++ = PATH_SEPARATOR;
                        len++;
                    }
                }
                *p2 = '\0';
                len += strlen( name );/* JBS 04/12/23 */
                if( len < _MAX_PATH ) {
                    strcat( p2, name );
                    /* check to see if file exists */
                    if( access( buffer, 0 ) == 0 ) {
                        __set_errno( prev_errno );
                        return;
                    }
                }
            }
            if( *p == '\0' ) break;
            ++p;
        }
    }
    buffer[0] = '\0';
}

/****************************************************************************
*
* Description:  Determine resource language from system environment.
*
****************************************************************************/

res_language_enumeration _WResLanguage(void)
{
    return( RLE_ENGLISH );
}

#ifdef __GLIBC__
/****************************************************************************
*
* Description:  Implementation of BSD style strlcpy().
*
****************************************************************************/

size_t strlcpy( char *dst, const char *src, size_t len )
{
    const char     *s;
    size_t              count;

    count = len;
    if( len ) {
        --len;                  // leave space for terminating null
        for( ; len; --len ) {
            if( *src == '\0' ) {
                break;
            }
            *dst++ = *src++;
        }
        *dst = '\0';            // terminate 'dst'
    } else {
        ++count;                // account for not decrementing 'len'
    }

    if( !len ) {                // source string was truncated
        s = src;
        while( *s != '\0' ) {
            ++s;
        }
        count += s - src;       // find out how long 'src' really is
    }
    return( count - len - 1 );  // final null doesn't count
}

/****************************************************************************
*
* Description:  Implementation of BSD style strlcat().
*
****************************************************************************/

size_t strlcat( char *dst, const char *t, size_t n )
{
    char   *s;
    size_t      len;

    s = dst;
    // Find end of string in destination buffer but don't overrun
    for( len = n; len; --len ) {
        if( *s == '\0' ) break;
        ++s;
    }
    // If no null char was found in dst, the buffer is messed up; don't
    // touch it
    if( *s == '\0' ) {
        --len;      // Decrement len to leave space for terminating null
        while( len != 0 ) {
            *s = *t;
            if( *s == '\0' ) {
                return( n - len - 1 );
            }
            ++s;
            ++t;
            --len;
        }
        // Buffer not large enough. Terminate and figure out desired length
        *s = '\0';
        while( *t++ != '\0' )
            ++n;
        --n;
    }
    return( n );
}
#endif
