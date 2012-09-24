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
#define _PIPEDUMP_LIB_LIBPIPEDUMP_C 1

///////////////
//           //
//  Headers  //
//           //
///////////////

#include "libpipedump.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/time.h>


/////////////////
//             //
//  Functions  //
//             //
/////////////////

#ifdef PMARK
#pragma mark - Pipe I/O
#endif

void pd_close_fd(pipedump_t * pd, int fildes)
{
   switch(fildes)
   {
      case PIPEDUMP_STDERR:
      if (pd->pipeerr != -1)
         close(pd->pipeerr);
      pd->pipeerr = -1;
      break;

      case PIPEDUMP_STDIN:
      if (pd->pipein != -1)
         close(pd->pipein);
      pd->pipein = -1;
      break;

      case PIPEDUMP_STDOUT:
      if (pd->pipeout != -1)
         close(pd->pipeout);
      pd->pipeout = -1;
      break;

      default:
      break;
   };
   return;
}


ssize_t pd_copy(pipedump_t * pd, int f1, int f2, void * buf, size_t nbyte)
{
   ssize_t len;
   assert((pd != NULL)  && "pd must not be NULL");
   assert((f1 != -1)    && "f1 must not be -1");
   assert((f2 != -1)    && "r2 must not be -1");
   assert((buf != NULL) && "buf must not be NULL");
   assert((nbyte != 0)  && "nbyte must not be 0");
   if ((len = pd_read(pd, f1, buf, nbyte)) == -1)
      return(-1);
   return(pd_write(pd, f2, buf, len));
}


ssize_t pd_read(pipedump_t * pd, int fildes, void * buf, size_t nbyte)
{
   assert((pd != NULL)   && "pd must not be NULL");
   assert((fildes != -1) && "f1 must not be -1");
   assert((buf != NULL)  && "buf must not be NULL");
   assert((nbyte != 0)   && "nbyte must not be 0");
   return(read(pd_fildes(pd, fildes), buf, nbyte));
}


ssize_t pd_write(pipedump_t * pd, int fildes, const void * buf, size_t nbyte)
{
   assert((pd != NULL)   && "pd must not be NULL");
   assert((fildes != -1) && "f1 must not be -1");
   assert((buf != NULL)  && "buf must not be NULL");
   assert((nbyte != 0)   && "nbyte must not be 0");
   return(write(pd_fildes(pd, fildes), buf, nbyte));
}



#ifdef PMARK
#pragma mark - Pipe Logging
#endif

int pd_closelog(pipedump_t * pd)
{
   assert((pd != NULL)   && "pd must not be NULL");
   if (pd->logfd == -1)
      return(0);
   close(pd->logfd);
   pd->logfd = -1;
   return(0);
}


int pd_log(pipedump_t * pd, const void * vbuf, size_t nbyte, int srcfd)
{
   // declare local vars
   unsigned         pos;
   struct timeval   tp;
   uint8_t          ipv6_header[40];
   uint8_t          udp_header[48];
   uint32_t         msb;
   uint32_t         lsb;
   uint32_t         pcap_len;
   uint32_t         udp_len;
   uint32_t         port;
   const uint8_t  * buff;
   uint8_t          header[48];

   struct
   {
      uint32_t       ts_sec;
      uint32_t       ts_usec;
      uint32_t       incl_len;
      uint32_t       orig_len;
   } pcap_header;

   assert((pd != NULL)   && "pd must not be NULL");
   assert((vbuf != NULL) && "buf must not be NULL");
   assert((nbyte != 0)   && "nbyte must not be 0");
   assert((srcfd != -1)  && "srcfd must not be -1");

   // casts void reference to uint8_t reference
   buff = vbuf;

   // grabs timestamp
   gettimeofday(&tp, NULL);

   // determines lengths
   udp_len  = (uint32_t) nbyte + 8;      // data + UDP header
   pcap_len = udp_len + 40;  // UDP length + IPv6 header

   // calculate port
   port = pd->pcap_sport + srcfd;

   // computes pcap header
   pcap_header.ts_sec   = (uint32_t) tp.tv_sec;
   pcap_header.ts_usec  = (uint32_t) tp.tv_usec;
   pcap_header.incl_len = (uint32_t) pcap_len;
   pcap_header.orig_len = (uint32_t) pcap_len;

   // computes IPv6 header
   memset(ipv6_header, 0, 40);
   ipv6_header[0] = 0x60;                     // version
   ipv6_header[4] = (udp_len >> 8) & 0xFF;    // Payload Length (byte 0)
   ipv6_header[5] = (udp_len >> 0) & 0xFF;    // Payload Length (byte 1)
   ipv6_header[6] = 17;                       // Next Header
   ipv6_header[7] = 7;                        // Hop Limit
   if (srcfd == PIPEDUMP_STDIN)
      ipv6_header[23] = 1;                    // Source Address
   else
      ipv6_header[39] = 1;                    // Destination Address

   // computes UDP header
   memset(udp_header, 0, 48);
   if (srcfd == PIPEDUMP_STDIN)
      udp_header[15] = 1;                     // Source Address
   else
      udp_header[31] = 1;                     // Destination Address
   udp_header[34] = (udp_len >> 8) & 0xFF;    // Payload Length (byte 3)
   udp_header[35] = (udp_len >> 0) & 0xFF;    // Payload Length (byte 4)
   udp_header[39] = 17;                       // Next Header
   udp_header[40] = (port >> 8) & 0xFF;       // Source Port (byte 0)
   udp_header[41] = (port >> 0) & 0xFF;       // Source Port (byte 1)
   udp_header[42] = (port >> 8) & 0xFF;       // Destination Port (byte 0)
   udp_header[43] = (port >> 0) & 0xFF;       // Destination Port (byte 1)
   udp_header[44] = (udp_len >> 8) & 0xFF;    // Length (byte 0)
   udp_header[45] = (udp_len >> 0) & 0xFF;    // Length (byte 1)

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
   for(pos = 0; pos < nbyte; pos += 2)
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
   udp_header[46] = msb;
   udp_header[47] = lsb;

   // writes data
   write(pd->logfd, &pcap_header, sizeof(pcap_header));
   write(pd->logfd, ipv6_header, 40);
   write(pd->logfd, &udp_header[40], 8);
   write(pd->logfd, buff, nbyte);

   return(0);
}


int pd_openlog(pipedump_t * pd, const char * logfile, mode_t mode)
{
   // declare local vars
   int      fd;
   unsigned uvar;
   int      var;

   assert((pd != NULL)      && "pd must not be NULL");
   assert((logfile != NULL) && "logfile must not be NULL");

   if (pd->logfd != -1)
      return(0);

   // open output log
   if ((fd = open(logfile, O_WRONLY|O_CREAT|O_TRUNC, mode)) == -1)
      return(-1);

   // print header (magic_number)
   uvar = 0xa1b2c3d4;
   write(fd, &uvar, sizeof(uvar));

   // print header (version_major/version_minor)
   uvar = 0x00020004;
   write(fd, &uvar, sizeof(uvar));

   // print header (thiszone)
   var = 0;
   write(fd, &var, sizeof(var));

   // print header (sigfigs)
   uvar = 0;
   write(fd, &uvar, sizeof(uvar));

   // print header (snaplen)
   uvar = 65535;
   write(fd, &uvar, sizeof(uvar));

   // print header (network)
   uvar = 101;
   write(fd, &uvar, sizeof(uvar));

   pd->logfd = fd;

   return(0);
}


#ifdef PMARK
#pragma mark - Pipe management
#endif

int pd_fildes(pipedump_t * pd, int fildes)
{
   switch(fildes)
   {
      case PIPEDUMP_STDERR:
      return(pd->pipeerr);

      case PIPEDUMP_STDIN:
      return(pd->pipein);

      case PIPEDUMP_STDOUT:
      return(pd->pipeout);

      default:
      break;
   };
   return(fildes);
}


void pd_free(pipedump_t ** pdp)
{
   int          pos;
   pipedump_t * pd;

   assert((pdp != NULL)  && "pd_free() cannot accept a NULL value");
   assert((*pdp != NULL) && "pd_free() accept a pointer referencing NULL");

   pd = *pdp;

   // close file descriptors
   if (pd->pipeerr != -1)
      close(pd->pipeerr);
   if (pd->pipein != -1)
      close(pd->pipein);
   if (pd->pipeout != -1)
      close(pd->pipeout);
   if (pd->logfd != -1)
      close(pd->logfd);

   // free argv
   if ((pd->argv))
   {
      for(pos = 0; pd->argv[pos]; pos++)
         free(pd->argv[pos]);
      free(pd->argv);
   };

   // free struct
   free(*pdp);
   *pdp = NULL;

   return;
}


int pd_get_option(pipedump_t * pd, int option, void * outvalue)
{
   assert((pd != NULL) && "pd_get_option() cannot accept a NULL value");
   switch(option)
   {
      case PIPEDUMP_PID:
      *((pid_t *)outvalue) = pd->pid;
      return(0);

      case PIPEDUMP_STDERR:
      *((int *)outvalue) = pd->pipeerr;
      return(0);

      case PIPEDUMP_STDIN:
      *((int *)outvalue) = pd->pipein;
      return(0);
      
      case PIPEDUMP_STDOUT:
      *((int *)outvalue) = pd->pipeout;
      return(0);

      default:
      break;
   };
   return(1);
}


pipedump_t * pd_init(const char *file, char *const argv[])
{
   pipedump_t * pd;
   int          eol;
   int          argc;

   assert((file != NULL) && "pd_open() must provide a valid pointer for *file");

   // allocates memory for pipe information
   if ((pd = calloc(sizeof(pipedump_t), 1)) == NULL)
      return(NULL);

   // sets default values
   pd->pipeerr    = -1;
   pd->pipein     = -1;
   pd->pipeout    = -1;
   pd->logfd      = -1;
   pd->pcap_sport = 19840;

   // sets addresses
   pd->lo_addr.s6_addr[15] = 1;

   // copies file to execute
   if ((pd->file = strdup(file)) == NULL)
   {
      pd_free(&pd);
      return(NULL);
   };

   // calculate argc
   argc = 1;
   if ((argv))
      for(argc = 0; argv[argc]; argc++);

   // allocate memory for argument list and strip double/single quotation marks
   if ((pd->argv = calloc(sizeof(char *), (argc+1))) == NULL)
   {
      pd_free(&pd);
      return(NULL);
   };

   // generate an argv list if one is not provided
   if (!(argv))
   {
      pd->argv[0] = strdup(file);
      if (!(pd->argv[0]))
      {
         pd_free(&pd);
         return(NULL);
      };
      return(pd);
   };

   // strips quotation marks
   for(pd->argc = 0; pd->argc < argc; pd->argc++)
   {
      // strips leading single/double quote
      if ((argv[pd->argc][0] == '"') || (argv[pd->argc][0] == '\''))
         pd->argv[pd->argc] = strdup(&argv[pd->argc][1]);
      else
         pd->argv[pd->argc] = strdup(&argv[pd->argc][0]);
      if (!(pd->argv[pd->argc]))
      {
         pd_free(&pd);
         return(NULL);
      };

      // strips tailing single/double quote
      eol = (int)strlen(pd->argv[pd->argc]);
      if ((pd->argv[pd->argc][eol-1] == '"') || (pd->argv[pd->argc][eol-1] == '\''))
         pd->argv[pd->argc][eol-1] = '\0';
   };

   return(pd);
}


int pd_fork(pipedump_t * pd)
{
   int          pipes[6];
   int          pos;

   // creates pipe for STDIN
   if ((pipe(&pipes[0 * 2])) == -1)
      return(-1);

   // creates pipe for STDOUT
   if ((pipe(&pipes[1 * 2])) == -1)
   {
      for(pos = 0; pos < 2; pos++)
         close(pipes[pos]);
      return(-1);
   };

   // creates pipe for STDERR
   if ((pipe(&pipes[2 * 2])) == -1)
   {
      for(pos = 0; pos < 4; pos++)
         close(pipes[pos]);
      return(-1);
   };

   // fork process
   switch(pd->pid = fork())
   {
      //
      // error forking process
      //
      case -1:
      for(pos = 0; pos < 6; pos++)
         close(pipes[pos]);
      return(-1);


      //
      // child process
      //
      case 0:
      // bind STDIN , STDOUT, and STDERR to pipes
      if ((pd->pipeerr = dup2(pipes[5], STDERR_FILENO)) == -1)
      {
         perror("dup2(pipe, STDERR_FILENO)");
         for(pos = 0; pos < 6; pos++)
            close(pipes[pos]);
         exit(1);
      };
      if ((pd->pipeout = dup2(pipes[3], STDOUT_FILENO)) == -1)
      {
         perror("dup2(pipe, STDOUT_FILENO)");
         for(pos = 0; pos < 6; pos++)
            close(pipes[pos]);
         exit(1);
      };
      if ((pd->pipein = dup2(pipes[0], STDIN_FILENO)) == -1)
      {
         perror("dup2(pipe, STDIN_FILENO)");
         for(pos = 0; pos < 6; pos++)
            close(pipes[pos]);
         exit(1);
      };
      // close master side of pipes
      close(pipes[1]);
      close(pipes[2]);
      close(pipes[4]);
      // execute command
      if ((execvp(pd->file, pd->argv)))
         perror("execvp()");
      pd_free(&pd);
      exit(1);

      //
      // master process
      //
      default:
      fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
      fcntl(pipes[2],     F_SETFL, O_NONBLOCK);
      fcntl(pipes[4],     F_SETFL, O_NONBLOCK);
      pd->pipein  = pipes[1];
      pd->pipeout = pipes[2];
      pd->pipeerr = pipes[4];
      close(pipes[0]);
      close(pipes[3]);
      close(pipes[5]);
      return(pd->pid);
   };

   return(-1);
}


#ifdef PMARK
#pragma mark - Library meta information
#endif

const char * pd_version(void)
{
#ifdef GIT_PACKAGE_VERSION
   return(GIT_PACKAGE_VERSION);
#else
   return("unknown");
#endif
}


/* end of source code */
