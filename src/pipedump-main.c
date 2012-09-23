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

#include <inttypes.h>
#include <stdio.h>
#include <getopt.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>


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
   int          fd;
   int          quiet;
   int          verbosity;
   int          start_port;
   const char * logfile;
};


//////////////////
//              //
//  Prototypes  //
//              //
//////////////////

// main statement
int main(int, char **);

int pipedump_log(pdconfig_t * cnf, const uint8_t * buff, size_t len, int source);
int pipedump_logclose(pdconfig_t * cnf);
int pipedump_logopen(pdconfig_t * cnf);

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
   pdconfig_t    cnf;

   // getopt options
   static char   short_opt[] = "ho:p:qvV";
   static struct option long_opt[] =
   {
      {"help",          no_argument, 0, 'h'},
      {"quiet",         no_argument, 0, 'q'},
      {"silent",        no_argument, 0, 'q'},
      {"verbose",       no_argument, 0, 'v'},
      {"version",       no_argument, 0, 'V'},
      {NULL,            0,           0, 0  }
   };

   // loops through arguments
   while((c = getopt_long(argc, argv, short_opt, long_opt, &opt_index)) != -1)
   {
      switch(c)
      {
         case -1:       // no more arguments
         case 0:        // long options toggles
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

   // open output log
   if (pipedump_logopen(&cnf) == -1)
      return(1);

   // closes files
   pipedump_logclose(&cnf);

   return(0);
}


int pipedump_log(pdconfig_t * cnf, const uint8_t * buff, size_t len, int source)
{
   // declare local vars
   unsigned       pos;
   struct timeval tp;
   uint8_t        ipv6_header[40];
   uint8_t        udp_header[48];
   uint32_t       msb;
   uint32_t       lsb;

   struct
   {
      uint32_t       ts_sec;
      uint32_t       ts_usec;
      uint32_t       incl_len;
      uint32_t       orig_len;
   } pcap_header;

   // grabs timestamp
   gettimeofday(&tp, NULL);

   // computes pcap header
   pcap_header.ts_sec   = (uint32_t) tp.tv_sec;
   pcap_header.ts_usec  = (uint32_t) tp.tv_usec;
   pcap_header.incl_len = (uint32_t) (len + 40 + 48); // data + IPv6 header + UDP header
   pcap_header.orig_len = pcap_header.incl_len;

   // computes IPv6 header
   memset(ipv6_header, 0, 40);
   ipv6_header[0] = 0x60;                     // version
   ipv6_header[4] = ((len + 48) >> 8) & 0xFF; // Payload Length (byte 0)
   ipv6_header[5] = ((len + 48) >> 0) & 0xFF; // Payload Length (byte 1)
   ipv6_header[6] = 17;                       // Next Header
   ipv6_header[7] = 7;                        // Hop Limit
   if (source == 0)
      ipv6_header[23] = 1;                    // Source Address
   else
      ipv6_header[39] = 1;                    // Destination Address

   // computes UDP header
   memset(udp_header, 0, 48);
   if (source == 0)
      udp_header[15] = 1;                     // Source Address
   else
      udp_header[31] = 1;                     // Destination Address
   udp_header[34] = ((len + 48) >> 8) & 0xFF; // Payload Length (byte 3)
   udp_header[35] = ((len + 48) >> 0) & 0xFF; // Payload Length (byte 4)
   udp_header[39] = 17;                       // Next Header
   udp_header[41] = source;                   // Source Port
   udp_header[43] = source;                   // Destination Port
   udp_header[44] = ((len + 48) >> 8) & 0xFF; // Length (byte 0)
   udp_header[45] = ((len + 48) >> 0) & 0xFF; // Length (byte 1)

   // computes checksum
   msb = 0;
   lsb = 0;
   for(pos = 0; pos < 48; pos += 2)
   {
      lsb = udp_header[pos+0] + (lsb & 0xFF) + (((msb & 0x100) == 1) ? 1 : 0);
      msb = (msb & 0xFF);
      msb = udp_header[pos+1] + (msb & 0xFF) + (((lsb & 0x100) == 1) ? 1 : 0);
      lsb = (lsb & 0xFF);
   }
   for(pos = 0; pos < len; pos += 2)
   {
      lsb = buff[pos+0] + (lsb & 0xFF) + (((msb & 0x100) == 1) ? 1 : 0);
      msb = (msb & 0xFF);
      msb = buff[pos+1] + (msb & 0xFF) + (((lsb & 0x100) == 1) ? 1 : 0);
      lsb = (lsb & 0xFF);
   }
   lsb = (lsb & 0xFF) + (((msb & 0x100) == 1) ? 1 : 0);
   msb = (msb & 0xFF);
   lsb = (lsb & 0xFF);
   if ((msb == 0) && (lsb == 0))
   {
      msb = 0xFF;
      lsb = 0xFF;
   };
   udp_header[18] = msb;
   udp_header[19] = lsb;

   // writes data
   write(cnf->fd, &pcap_header, sizeof(pcap_header));
   write(cnf->fd, ipv6_header, 40);
   write(cnf->fd, udp_header, 48);
   write(cnf->fd, buff, len);

   return(0);
}


int pipedump_logclose(pdconfig_t * cnf)
{
   if (cnf->fd == -1)
      return(0);
   close(cnf->fd);
   return(0);
}


int pipedump_logopen(pdconfig_t * cnf)
{
   // declare local vars
   unsigned uvar;
   int      var;

   // open output log
   if ((cnf->verbosity))
      fprintf(stderr, "%s: opening \"%s\" for writing...\n", PROGRAM_NAME, cnf->logfile);
   if ((cnf->fd = open(cnf->logfile, O_WRONLY|O_CREAT|O_TRUNC, 0644)) == -1)
   {
      perror("open()");
      return(-1);
   };

   // print header (magic_number)
   uvar = 0xa1b2c3d4;
   write(cnf->fd, &uvar, sizeof(uvar));

   // print header (version_major/version_minor)
   uvar = (2 << 16) & (4 << 0);
   write(cnf->fd, &uvar, sizeof(uvar));

   // print header (thiszone)
   var = 0;
   write(cnf->fd, &uvar, sizeof(var));

   // print header (sigfigs)
   uvar = 0;
   write(cnf->fd, &uvar, sizeof(uvar));

   // print header (snaplen)
   uvar = 65535;
   write(cnf->fd, &uvar, sizeof(uvar));

   // print header (network)
   uvar = 101;
   write(cnf->fd, &uvar, sizeof(uvar));

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
