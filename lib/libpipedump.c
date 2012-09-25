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
#include <sys/stat.h>
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
   return(((len)) ? pd_write(pd, f2, buf, len) : 0);
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


int pd_log(pipedump_t * pd, const void * buf, size_t nbyte, int srcfd)
{
   // declare local vars
   struct timeval   tp;
   uint32_t         port;
   uint8_t        * src_addr;
   uint8_t        * dst_addr;
   union
   {
      struct
      {
         // PCAP headers
         uint32_t       pcap_ts_sec;
         uint32_t       pcap_ts_usec;
         uint32_t       pcap_incl_len;
         uint32_t       pcap_orig_len;

         // IPv6 headers
         uint8_t        ipv6_version;
         uint8_t        ipv6_traffic_class;
         uint8_t        ipv6_flow_label[2];
         uint8_t        ipv6_payload_length[2];
         uint8_t        ipv6_next_header;
         uint8_t        ipv6_hop_limit;
         uint8_t        ipv6_src_addr[16];
         uint8_t        ipv6_dst_addr[16];

         // UDP headers
         uint8_t        udp_src_port[2];
         uint8_t        udp_dst_port[2];
         uint8_t        udp_length[2];
         uint8_t        udp_checksum[2];
      } members;
      uint8_t bytes[64];
   } header;

   assert((pd != NULL)   && "pd must not be NULL");
   assert((buf != NULL) && "buf must not be NULL");
   assert((nbyte != 0)   && "nbyte must not be 0");
   assert((srcfd != -1)  && "srcfd must not be -1");

   // grabs timestamp
   gettimeofday(&tp, NULL);

   // calculate addresses
   if ((srcfd == PIPEDUMP_STDIN) || (srcfd == STDIN_FILENO))
   {
      src_addr = pd->lo_addr.s6_addr;
      dst_addr = pd->rm_addr.s6_addr;
   } else {
      src_addr = pd->rm_addr.s6_addr;
      dst_addr = pd->lo_addr.s6_addr;
   };

   // calculate port
   port = srcfd;
   if (srcfd < 3)
      port = pd->pcap_sport + srcfd;

   // clears header
   memset(header.bytes, 0, 64);

   // computes pcap header
   header.members.pcap_ts_sec   = (uint32_t) tp.tv_sec;
   header.members.pcap_ts_usec  = (uint32_t) tp.tv_usec;
   header.members.pcap_incl_len = (uint32_t) (nbyte + 48);
   header.members.pcap_orig_len = (uint32_t) (nbyte + 48);

   // computes IPv6 header
   header.members.ipv6_version            = 0x60;
   header.members.ipv6_payload_length[0]  = ((nbyte + 8) >> 8) & 0xFF;
   header.members.ipv6_payload_length[1]  = ((nbyte + 8) >> 0) & 0xFF;
   header.members.ipv6_next_header        = 17;
   header.members.ipv6_hop_limit          = 7;
   memcpy(header.members.ipv6_src_addr, src_addr, 16);
   memcpy(header.members.ipv6_dst_addr, dst_addr, 16);

   // computes UDP header
   header.members.udp_src_port[0] = (port >> 8) & 0xFF;
   header.members.udp_src_port[1] = (port >> 0) & 0xFF;
   header.members.udp_dst_port[0] = (port >> 8) & 0xFF;
   header.members.udp_dst_port[1] = (port >> 0) & 0xFF;
   header.members.udp_length[0]   = ((nbyte + 8) >> 8) & 0xFF;
   header.members.udp_length[1]   = ((nbyte + 8) >> 0) & 0xFF;
   header.members.udp_checksum[0] = 0xFF;
   header.members.udp_checksum[1] = 0xFF;

   // computes checksum
   // Needs to be written

   // writes data to logfile
   write(pd->logfd, header.bytes, 64);
   write(pd->logfd, (const uint8_t *)buf, nbyte);

   return(0);
}


int pd_openlog(pipedump_t * pd, const char * logfile, mode_t mode)
{
   // declare local vars
   int         fd;
   struct stat sb;
   int         err;
   union
   {
      struct
      {
         uint32_t       magic_number;
         uint32_t       version;
         int32_t        thiszone;
         uint32_t       sigfigs;
         uint32_t       snaplen;
         uint32_t       network;
      } members;
      uint8_t bytes[24];
   } header;

   assert((pd != NULL)      && "pd must not be NULL");
   assert((logfile != NULL) && "logfile must not be NULL");

   if (pd->logfd != -1)
      return(0);

   // attempts to stat file
   err = stat(logfile, &sb);

   // open output log
   if ((fd = open(logfile, O_WRONLY|O_CREAT|O_APPEND, mode)) == -1)
      return(-1);

   // calculate pcap header
   header.members.magic_number = 0xa1b2c3d4;
   header.members.version      = 0x00020004;  // 2.4
   header.members.thiszone     = 0;
   header.members.sigfigs      = 0;
   header.members.snaplen      = 65535;
   header.members.network      = 101;         // LINKTYPE_RAW

   // write header to log file
   if (err == -1)
   {
      if (write(fd, header.bytes, 24) == -1)
      {
         close(fd);
         return(-1);
      };
   };

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

      case PIPEDUMP_START_PORT:
      *((int *)outvalue) = pd->pcap_sport;
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


int pd_set_option(pipedump_t * pd, int option, void * outvalue)
{
   assert((pd != NULL) && "pd_get_option() cannot accept a NULL value");
   switch(option)
   {
      case PIPEDUMP_START_PORT:
      pd->pcap_sport = *((int *)outvalue);
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
