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



/////////////////
//             //
//  Functions  //
//             //
/////////////////


void pd_close(pipedump_t ** pdp)
{
   assert((pdp != NULL) && "pd_close() cannot accept a NULL value");
   assert((*pdp != NULL) && "pd_close() accept a pointer referencing NULL");
   if ((*pdp)->pipeerr != -1)
      close((*pdp)->pipeerr);
   if ((*pdp)->pipein != -1)
      close((*pdp)->pipein);
   if ((*pdp)->pipeout != -1)
      close((*pdp)->pipeout);
   free(*pdp);
   *pdp = NULL;
   return;
}


void pd_close_fd(pipedump_t * pd, int option)
{
   switch(option)
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


pipedump_t * pd_open(const char *file, char *const argv[])
{
   pipedump_t * pd;
   int          pipes[6];
   int          pos;
   char      ** targv;
   int          argc;
   int          eol;

   assert((file != NULL) && "pd_open() must provide a valid pointer for *file");
   assert((argv != NULL) && "pd_open() must provide a valid pointer for *argv[]");

   // allocates memory for pipe information
   if ((pd = calloc(sizeof(pipedump_t), 1)) == NULL)
      return(NULL);

   // creates pipe for STDIN
   if ((pipe(&pipes[0 * 2])) == -1)
   {
      free(pd);
      return(NULL);
   };

   // creates pipe for STDOUT
   if ((pipe(&pipes[1 * 2])) == -1)
   {
      free(pd);
      for(pos = 0; pos < 2; pos++)
         close(pipes[pos]);
      return(NULL);
   };

   // creates pipe for STDERR
   if ((pipe(&pipes[2 * 2])) == -1)
   {
      free(pd);
      for(pos = 0; pos < 4; pos++)
         close(pipes[pos]);
      return(NULL);
   };

   // fork process
   switch(pd->pid = fork())
   {
      // error forking process
      case -1:
      for(pos = 0; pos < 6; pos++)
         close(pipes[pos]);
      free(pd);
      return(NULL);

      // child process
      case 0:
      if ((pd->pipein = dup2(pipes[0], STDIN_FILENO)) == -1)
      {
         for(pos = 0; pos < 6; pos++)
            close(pipes[pos]);
         free(pd);
         return(NULL);
      };
      if ((pd->pipeout = dup2(pipes[3], STDOUT_FILENO)) == -1)
      {
         for(pos = 0; pos < 6; pos++)
            close(pipes[pos]);
         free(pd);
         return(NULL);
      };
      if ((pd->pipeerr = dup2(pipes[5], STDERR_FILENO)) == -1)
      {
         for(pos = 0; pos < 6; pos++)
            close(pipes[pos]);
         free(pd);
         return(NULL);
      };
      pd->pipein  = pipes[0];
      pd->pipeout = pipes[3];
      pd->pipeerr = pipes[5];
      close(pipes[1]);
      close(pipes[2]);
      close(pipes[4]);
      for(argc = 0; argv[argc]; argc++);
      if ((targv = calloc(sizeof(char *), argc)) == NULL)
      {
         fprintf(stderr, "out of virtual memory\n");
         exit(1);
      };
      for(pos = 0; pos < argc; pos++)
      {
         if ((argv[pos][0] == '"') || (argv[pos][0] == '\''))
            targv[pos] = strdup(&argv[pos][1]);
         else
            targv[pos] = strdup(&argv[pos][0]);
         eol = (int)strlen(targv[pos]);
         if ((targv[pos][eol-1] == '"') || (targv[pos][eol-1] == '\''))
            targv[pos][eol-1] = '\0';
      };
      if ((execvp(file, targv)))
         perror("execvp()");
      pd_close(&pd);
      exit(0);

      // master process
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
      return(pd);
   };

   return(NULL);
}


const char * pd_version(void)
{
#ifdef GIT_PACKAGE_VERSION
   return(GIT_PACKAGE_VERSION);
#else
   return("unknown");
#endif
}


/* end of source code */
