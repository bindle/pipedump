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

extern char **environ;


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
#ifndef GIT_PACKAGE_STRING
#define GIT_PACKAGE_STRING ""
#endif
#undef PACKAGE_VERSION
#define PACKAGE_VERSION GIT_PACKAGE_STRING


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
   int             c;
   int             opt_index;
   char         ** pargv;
   int             pos;
   int             ret;
   ssize_t         len;
   pipedump_t    * pd;
   pid_t           pid;
   struct pollfd   pollfd[3];
   int             pollfd_len;
   int             verbosity;
   int             debug;
   int             start_port;
   const char    * logfile;
   uint8_t       * buff;
   size_t          buff_len;

   // getopt options
   static char   short_opt[] = "b:dho:p:qvV";
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
   start_port = 19840;
   verbosity  = 0;
   logfile    = NULL;
   debug      = 0;
   buff_len   = 0;

   // loops through arguments
   while((c = getopt_long(argc, argv, short_opt, long_opt, &opt_index)) != -1)
   {
      switch(c)
      {
         case -1:       // no more arguments
         case 0:        // long options toggles
         break;

         case 'b':
         buff_len = atol(optarg);
         break;

         case 'd':
         debug++;
         break;

         case 'h':
         pipedump_usage();
         return(0);

         case 'o':
         logfile = optarg;
         break;

         case 'p':
         start_port = (int)atol(optarg);
         break;

         case 'q':
         break;

         case 'V':
         pipedump_version();
         return(0);

         case 'v':
         verbosity++;
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
   if (!(buff_len))
      buff_len = 4096;
   if ((verbosity))
      fprintf(stderr, "%s: allocating %i byte buffer...\n", PROGRAM_NAME, (int)buff_len);
   if ((buff = malloc(buff_len)) == NULL)
   {
      fprintf(stderr, "%s: out of virtual memory\n", PROGRAM_NAME);
      return(1);
   };

   // initializes pipedump instance
   if ((pd = pd_init(pargv[0], pargv)) == NULL)
   {
      perror("pd_init()");
      free(buff);
      return(1);
   };
   pd_set_option(pd, PIPEDUMP_START_PORT, &start_port);

   // open output log
   if ((verbosity))
      fprintf(stderr, "%s: opening logfile \"%s\"...\n", PROGRAM_NAME, logfile);
   if (pd_openlog(pd, logfile, 0644) == -1)
   {
      perror("pd_openlog()");
      free(buff);
      pd_free(&pd);
      return(1);
   };

   // logs debug information
   if ((debug))
   {
      // logs pipedump version
      len = snprintf((char *)buff, 1024, "%s (%s) %s", PROGRAM_NAME, PACKAGE_NAME, PACKAGE_VERSION);
      pd_log(pd, buff, len, 0xFFFF);

      // logs environment variables
      if (debug > 1)
         for(pos = 0; environ[pos]; pos++)
            pd_log(pd, environ[pos], strlen(environ[pos]), 0xFFFF);

      // logs command
      buff[0] = '\0';
      strncat((char *)buff, "Executing: ", 1024);
      for(pos = 0; pargv[pos]; pos++)
      {
         strncat((char *)buff, pargv[pos], 1024);
         strncat((char *)buff, " ", 1024);
      };
      pd_log(pd, buff, strlen((char *)buff), 0xFFFF);
   };

   // executes command
   if ((verbosity))
      fprintf(stderr, "%s: executing \"%s\"...\n", PROGRAM_NAME, pargv[0]);
   if (pd_fork(pd) == -1)
   {
      perror("pd_fork()");
      free(buff);
      pd_free(&pd);
      return(1);
   };

   // setups polling info
   if ((verbosity))
      fprintf(stderr, "%s: saving pipes...\n", PROGRAM_NAME);
   pd_get_option(pd, PIPEDUMP_PID,    &pid);
   pollfd[2].fd = STDIN_FILENO;
   pollfd[1].fd = pd_fildes(pd, PIPEDUMP_STDOUT);
   pollfd[0].fd = pd_fildes(pd, PIPEDUMP_STDERR);
   pollfd_len = 3;
   for(pos = 0; pos < pollfd_len; pos++)
      pollfd[pos].events = POLLIN | POLLPRI | POLLERR | POLLHUP | POLLRDBAND;

   // loops until exit or error
   if ((verbosity))
      fprintf(stderr, "%s: entering run loop...\n", PROGRAM_NAME);
   while ((ret = poll(pollfd, pollfd_len, -1)) > 0)
   {
      // checks STDIN for data
      if (pollfd[2].revents & (POLLPRI|POLLIN))
      {
         while((len = pd_copy(pd, pollfd[2].fd, PIPEDUMP_STDIN, buff, buff_len)) > 0)
         {
            if ((verbosity))
               fprintf(stderr, "%s: logging %i bytes for STDIN\n", PROGRAM_NAME, (int)len);
            pd_log(pd, buff, len, 0);
         };
      };

      // checks PIPEOUT for data
      if (pollfd[1].revents & (POLLPRI|POLLIN))
      {
         while((len = pd_copy(pd, pollfd[1].fd, STDOUT_FILENO, buff, buff_len)) > 0)
         {
            if ((verbosity))
               fprintf(stderr, "%s: logging %i bytes for STDOUT\n", PROGRAM_NAME, (int)len);
            pd_log(pd, buff, len, 1);
         };
      };

      // checks PIPEERR for data
      if (pollfd[0].revents & (POLLPRI|POLLIN))
      {
         while((len = pd_copy(pd, pollfd[2].fd, STDERR_FILENO, buff, buff_len)) > 0)
         {
            if ((verbosity))
               fprintf(stderr, "%s: logging %i bytes for STDERR\n", PROGRAM_NAME, (int)len);
            pd_log(pd, buff, len, 2);
         };
      };

      // close pipe if STDIN is closed
      if (pollfd[2].revents & POLLHUP)
      {
         if ((verbosity))
            fprintf(stderr, "%s: closing STDIN\n", PROGRAM_NAME);
         pd_close_fd(pd, PIPEDUMP_STDIN);
         pollfd_len = 2;
         pollfd[2].revents = 0;
      };

      // exits if any of the pipes close
      if ( (pollfd[0].revents & POLLHUP) || (pollfd[1].revents & POLLHUP) )
      {
         if ((waitpid(pid, &ret, 0)) == -1)
            perror("waitid()");

         // return with exit code of child
         if ((verbosity))
            fprintf(stderr, "%s: exiting with status %i\n", PROGRAM_NAME, WEXITSTATUS(ret));
         free(buff);
         pd_free(&pd);
         return(WEXITSTATUS(ret));
      };
   };

   // closes files
   free(buff);
   pd_free(&pd);

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
         "  -d                        enable debug output registered on port 65535)\n"
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
