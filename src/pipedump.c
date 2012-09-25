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
#define _PIPEDUMP_SRC_PIPEDUMP_MAIN_C 1

///////////////
//           //
//  Headers  //
//           //
///////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pipedump.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <poll.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/wait.h>


///////////////////
//               //
//  i18l Support //
//               //
///////////////////

#ifdef HAVE_GETTEXT
#   include <gettext.h>
#   include <libintl.h>
#   define _(String) gettext (String)
#   define gettext_noop(String) String
#   define N_(String) gettext_noop (String)
#else
#   define _(String) (String)
#   define N_(String) String
#   define textdomain(Domain)
#   define bindtextdomain(Package, Directory)
#endif


///////////////////
//               //
//  Definitions  //
//               //
///////////////////

#ifndef PROGRAM_NAME
#define PROGRAM_NAME "pipedump"
#endif
#ifndef PACKAGE_BUGREPORT
#define PACKAGE_BUGREPORT "syzdek@bindlebinaries.com"
#endif
#ifndef PACKAGE_COPYRIGHT
#define PACKAGE_COPYRIGHT ""
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME ""
#endif
#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION ""
#endif


/////////////////
//             //
//  Datatypes  //
//             //
/////////////////

/// tag data
typedef struct pipedump_config pdconfig_t;
struct pipedump_config
{
   pipedump_t    * pd;
   int             pd_in;
   int             fd;
   pid_t           pid;
   struct pollfd   pollfd[3];
   int             quiet;
   int             verbosity;
   int             start_port;
   const char    * logfile;
   uint8_t       * buff;
   size_t          buff_len;
};


//////////////////
//              //
//  Prototypes  //
//              //
//////////////////

// main statement
int main(int, char **);

// displays usage
void pipedump_usage(void);

// displays usage
void pipedump_version(void);


/////////////////
//             //
//  Functions  //
//             //
/////////////////

/// start of blipsd
/// @param[in] argc   number of arguments
/// @param[in] argv   array of arguments
int main(int argc, char * argv[])
{
   // declare local vars
   int           c;
   int           opt_index;
   char       ** pargv;
   int           pos;
   int           ret;
   char          buff[1024];
   ssize_t       len;
   pdconfig_t    cnf;

   // getopt options
   static char   short_opt[] = "b:ho:p:qvV";
   static struct option long_opt[] =
   {
      {"help",          no_argument, 0, 'h'},
      {"quiet",         no_argument, 0, 'q'},
      {"silent",        no_argument, 0, 'q'},
      {"verbose",       no_argument, 0, 'v'},
      {"version",       no_argument, 0, 'V'},
      {NULL,            0,           0, 0  }
   };

   // sets default values
   memset(&cnf, 0, sizeof(cnf));
   cnf.start_port = 19840;

   // loops through arguments
   while((c = getopt_long(argc, argv, short_opt, long_opt, &opt_index)) != -1)
   {
      switch(c)
      {
         case -1:       // no more arguments
         case 0:        // long options toggles
         break;

         case 'b':
         cnf.buff_len = atol(optarg);
         break;

         case 'h':
         pipedump_usage();
         return(0);

         case 'o':
         cnf.logfile = optarg;
         break;

         case 'p':
         cnf.start_port = (int)atol(optarg);
         break;

         case 'q':
         cnf.quiet = 1;
         break;

         case 'V':
         pipedump_version();
         return(0);

         case 'v':
         cnf.verbosity++;
         break;

         case '?':
         fprintf(stderr, _("Try `%s --help' for more information.\n"), PROGRAM_NAME);
         return(1);

         default:
         fprintf(stderr, _("%s: unrecognized option `--%c'\n"
               "Try `%s --help' for more information.\n"
            ),  PROGRAM_NAME, c, PROGRAM_NAME
         );
         return(1);
      };
   };

   // checks arguments
   if (argc <= optind)
   {
      fprintf(stderr, _("%s: missing required command\n"
                        "Try `%s --help' for more information.\n"),
                        PROGRAM_NAME, PROGRAM_NAME
      );
      return(1);
   };
   pargv = &argv[optind];

   // allocates buffer
   if (!(cnf.buff_len))
      cnf.buff_len = 4096;
   if ((cnf.verbosity))
      fprintf(stderr, "%s: allocating %i byte buffer...\n", PROGRAM_NAME, (int)cnf.buff_len);
   if ((cnf.buff = malloc(cnf.buff_len)) == NULL)
   {
      fprintf(stderr, "%s: out of virtual memory\n", PROGRAM_NAME);
      return(1);
   };

   // initializes pipedump instance
   if ((cnf.pd = pd_init(pargv[0], pargv)) == NULL)
   {
      perror("pd_init()");
      return(1);
   };

   // open output log
   if ((cnf.verbosity))
      fprintf(stderr, "%s: opening logfile \"%s\"...\n", PROGRAM_NAME, cnf.logfile);
   if ((cnf.fd = pd_openlog(cnf.pd, cnf.logfile, 0644)) == -1)
   {
      perror("pd_openlog()");
      pd_free(&cnf.pd);
      return(1);
   };

   // logs command to be executed
   len = snprintf(buff, 1024, "%s (%s) %s", PROGRAM_NAME, PACKAGE_NAME, PACKAGE_VERSION);
   pd_log(cnf.pd, buff, len, 0xFFFF);
   buff[0] = '\0';
   strncat(buff, "Executing: ", 1024);
   for(pos = 0; pargv[pos]; pos++)
   {
      strncat(buff, pargv[pos], 1024);
      strncat(buff, " ", 1024);
   };
   pd_log(cnf.pd, buff, strlen(buff), 0xFFFF);

   // executes command
   if ((cnf.verbosity))
      fprintf(stderr, "%s: executing \"%s\"...\n", PROGRAM_NAME, pargv[0]);
   if (pd_fork(cnf.pd) == -1)
   {
      perror("pd_fork()");
      pd_free(&cnf.pd);
      return(1);
   };

   // setups polling info
   if ((cnf.verbosity))
      fprintf(stderr, "%s: saving pipes...\n", PROGRAM_NAME);
   pd_get_option(cnf.pd, PIPEDUMP_PID,    &cnf.pid);
   cnf.pollfd[0].fd = STDIN_FILENO;
   cnf.pollfd[1].fd = pd_fildes(cnf.pd, PIPEDUMP_STDOUT);
   cnf.pollfd[2].fd = pd_fildes(cnf.pd, PIPEDUMP_STDERR);
   for(pos = 0; pos < 3; pos++)
      cnf.pollfd[pos].events = POLLIN | POLLPRI | POLLERR | POLLHUP | POLLRDBAND;

   // loops until exit or error
   if ((cnf.verbosity))
      fprintf(stderr, "%s: entering run loop...\n", PROGRAM_NAME);
   while ((ret = poll(cnf.pollfd, 3, -1)) > 0)
   {
      // checks STDIN for data
      if (cnf.pollfd[0].revents & (POLLPRI|POLLIN))
      {
         while((len = read(STDIN_FILENO, cnf.buff, cnf.buff_len)) > 0)
         {
            if ((cnf.verbosity))
               fprintf(stderr, "%s: logging %i bytes for STDIN\n", PROGRAM_NAME, (int)len);
            pd_write(cnf.pd, PIPEDUMP_STDIN, cnf.buff, len);
            pd_log(cnf.pd, cnf.buff, len, 0);
         };
      };

      // checks PIPEOUT for data
      if (cnf.pollfd[1].revents & (POLLPRI|POLLIN))
      {
         while((len = read(cnf.pollfd[1].fd, cnf.buff, cnf.buff_len)) > 0)
         {
            if ((cnf.verbosity))
               fprintf(stderr, "%s: logging %i bytes for STDOUT\n", PROGRAM_NAME, (int)len);
            pd_write(cnf.pd, STDOUT_FILENO, cnf.buff, len);
            pd_log(cnf.pd, cnf.buff, len, 1);
         };
      };

      // checks PIPEERR for data
      if (cnf.pollfd[2].revents & (POLLPRI|POLLIN))
      {
         while((len = read(cnf.pollfd[2].fd, cnf.buff, cnf.buff_len)) > 0)
         {
            if ((cnf.verbosity))
               fprintf(stderr, "%s: logging %i bytes for STDERR\n", PROGRAM_NAME, (int)len);
            pd_write(cnf.pd, STDERR_FILENO, cnf.buff, len);
            pd_log(cnf.pd, cnf.buff, len, 2);
         };
      };

      // close pipe if STDIN is closed
      if (cnf.pollfd[0].revents & POLLHUP)
      {
         if ((cnf.verbosity))
            fprintf(stderr, "%s: closing STDIN\n", PROGRAM_NAME);
         pd_close_fd(cnf.pd, PIPEDUMP_STDIN);
      };

      // exits if any of the pipes close
      if ( (cnf.pollfd[1].revents & POLLHUP) || (cnf.pollfd[2].revents & POLLHUP) )
      {
         if ((waitpid(cnf.pid, &ret, 0)) == -1)
            perror("waitid()");

         // return with exit code of child
         if ((cnf.verbosity))
            fprintf(stderr, "%s: exiting with status %i\n", PROGRAM_NAME, WEXITSTATUS(ret));
         pd_free(&cnf.pd);
         return(WEXITSTATUS(ret));
      };
   };

   // closes files
   pd_free(&cnf.pd);

   return(0);
}


/// displays usage
void pipedump_usage(void)
{
   // TRANSLATORS: The following strings provide usage for command. These
   // strings are displayed if the program is passed `--help' on the command
   // line. The two strings referenced are: PROGRAM_NAME, and
   // PACKAGE_BUGREPORT
   printf(_("Usage: %s [OPTIONS] -- command\n"
         "  -b length                 length in bytes of buffer to allocate(default 4096)\n"
         "  -h, --help                print this help and exit\n"
         "  -o file                   output file\n"
         "  -p port                   starting port number (default 19840)\n"
         "  -q, --quiet, --silent     do not print messages\n"
         "  -v, --verbose             print verbose messages\n"
         "  -V, --version             print version number and exit\n"
         "\n"
         "Report bugs to <%s>.\n"
      ), PROGRAM_NAME, PACKAGE_BUGREPORT
   );
   return;
}


/// displays version
void pipedump_version(void)
{
   // TRANSLATORS: The following strings provide version and copyright
   // information if the program is passed --version on the command line.
   // The three strings referenced are: PROGRAM_NAME, PACKAGE_NAME,
   // PACKAGE_VERSION.
   printf(_("%s (%s) %s\n"
         "Written by David M. Syzdek.\n"
         "\n"
         "%s\n"
         "This is free software; see the source for copying conditions.  There is NO\n"
         "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
      ), PROGRAM_NAME, PACKAGE_NAME, PACKAGE_VERSION, PACKAGE_COPYRIGHT
   );
   return;
}


/* end of source code */
