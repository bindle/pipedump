/*
 *  Pipe Dump Utility & Library
 *  Copyright (C) 2012 Bindle Binaries <syzdek@bindlebinaries.com>.
 *
 *  @BINDLE_BINARIES_BSD_LICENSE_START@
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Bindle Binaries nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 *  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL BINDLE BINARIES BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 *  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *  SUCH DAMAGE.
 *
 *  @BINDLE_BINARIES_BSD_LICENSE_END@
 */
/**
 *  @file include/pipedump.h
 *  Pipe Dump Library API
 */
#ifndef _PIPEDUMP_H
#define _PIPEDUMP_H 1


///////////////
//           //
//  Headers  //
//           //
///////////////

#include <sys/types.h>


//////////////
//          //
//  Macros  //
//          //
//////////////

/*
 * The macros "BEGIN_C_DECLS" and "END_C_DECLS" are taken verbatim
 * from section 7.1 of the Libtool 1.5.14 manual.
 */
/* BEGIN_C_DECLS should be used at the beginning of your declarations,
   so that C++ compilers don't mangle their names. Use END_C_DECLS at
   the end of C declarations. */
#undef BEGIN_C_DECLS
#undef END_C_DECLS
#if defined(__cplusplus) || defined(c_plusplus)
#   define BEGIN_C_DECLS  extern "C" {    ///< exports as C functions
#   define END_C_DECLS    }               ///< exports as C functions
#else
#   define BEGIN_C_DECLS  /* empty */     ///< exports as C functions
#   define END_C_DECLS    /* empty */     ///< exports as C functions
#endif


/*
 * The macro "PARAMS" is taken verbatim from section 7.1 of the
 * Libtool 1.5.14 manual.
 */
/* PARAMS is a macro used to wrap function prototypes, so that
   compilers that don't understand ANSI C prototypes still work,
   and ANSI C compilers can issue warnings about type mismatches. */
#undef PARAMS
#if defined (__STDC__) || defined (_AIX) \
        || (defined (__mips) && defined (_SYSTYPE_SVR4)) \
        || defined(WIN32) || defined (__cplusplus)
# define PARAMS(protos) protos   ///< wraps function arguments in order to support ANSI C
#else
# define PARAMS(protos) ()       ///< wraps function arguments in order to support ANSI C
#endif


/*
 * The following macro is taken verbatim from section 5.40 of the GCC
 * 4.0.2 manual.
 */
#if __STDC_VERSION__ < 199901L
# if __GNUC__ >= 2
# define __func__ __FUNCTION__
# else
# define __func__ "<unknown>"
# endif
#endif


// Exports function type
#ifdef WIN32
#   ifdef PIPEDUMP_LIBS_DYNAMIC
#      define PIPEDUMP_F(type)   extern __declspec(dllexport) type   ///< used for library calls
#      define PIPEDUMP_V(type)   extern __declspec(dllexport) type   ///< used for library calls
#   else
#      define PIPEDUMP_F(type)   extern __declspec(dllimport) type   ///< used for library calls
#      define PIPEDUMP_V(type)   extern __declspec(dllimport) type   ///< used for library calls
#   endif
#else
#   ifdef PIPEDUMP_LIBS_DYNAMIC
#      define PIPEDUMP_F(type)   type                                ///< used for library calls
#      define PIPEDUMP_V(type)   type                                ///< used for library calls
#   else
#      define PIPEDUMP_F(type)   extern type                         ///< used for library calls
#      define PIPEDUMP_V(type)   extern type                         ///< used for library calls
#   endif
#endif


// Pipe Dump Options
#define PIPEDUMP_STDIN    -10
#define PIPEDUMP_STDOUT   -11
#define PIPEDUMP_STDERR   -12
#define PIPEDUMP_PID      8


/////////////////
//             //
//  Datatypes  //
//             //
/////////////////

/// pipedump_t contains state information about the forked process.
typedef struct pipedump_state pipedump_t;


//////////////////
//              //
//  Prototypes  //
//              //
//////////////////
BEGIN_C_DECLS

/// @name Pipe I/O
PIPEDUMP_F(void) pd_close_fd(pipedump_t * pd, int fildes);
PIPEDUMP_F(ssize_t) pd_copy(pipedump_t * pd, int f1, int f2, void * buf, size_t nbyte);
PIPEDUMP_F(ssize_t) pd_read(pipedump_t * pd, int fildes, void * buf, size_t nbyte);
PIPEDUMP_F(ssize_t) pd_write(pipedump_t * pd, int fildes, const void * buf, size_t nbyte);

/// @name Pipe Logging
PIPEDUMP_F(int) pd_openlog(pipedump_t * pd, const char * logfile, mode_t mode);
PIPEDUMP_F(int) pd_closelog(pipedump_t * pd);
PIPEDUMP_F(int) pd_log(pipedump_t * pd, const void * buff, size_t nbyte, int tag);

/// @name Pipe management
PIPEDUMP_F(int) pd_fildes(pipedump_t * pd, int fildes);
PIPEDUMP_F(void) pd_free(pipedump_t ** pdp);
PIPEDUMP_F(int) pd_get_option(pipedump_t * pd, int option, void * outvalue);
PIPEDUMP_F(pipedump_t *) pd_init(const char * file, char * const argv[]);
PIPEDUMP_F(int) pd_fork(pipedump_t * pd);

/// @name Library meta information
PIPEDUMP_F(const char *) pd_version(void);

END_C_DECLS
#endif /* end of header file */
