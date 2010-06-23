/*
  File: path.c
  Author: James Oakley
  Copyright (C): 2010 Dartmouth College
  License: Katana is free software: you may redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version. Regardless of
    which version is chose, the following stipulation also applies:
    
    Any redistribution must include copyright notice attribution to
    Dartmouth College as well as the Warranty Disclaimer below, as well as
    this list of conditions in any related documentation and, if feasible,
    on the redistributed software; Any redistribution must include the
    acknowledgment, “This product includes software developed by Dartmouth
    College,” in any related documentation and, if feasible, in the
    redistributed software; and The names “Dartmouth” and “Dartmouth
    College” may not be used to endorse or promote products derived from
    this software.  

                             WARRANTY DISCLAIMER

    PLEASE BE ADVISED THAT THERE IS NO WARRANTY PROVIDED WITH THIS
    SOFTWARE, TO THE EXTENT PERMITTED BY APPLICABLE LAW. EXCEPT WHEN
    OTHERWISE STATED IN WRITING, DARTMOUTH COLLEGE, ANY OTHER COPYRIGHT
    HOLDERS, AND/OR OTHER PARTIES PROVIDING OR DISTRIBUTING THE SOFTWARE,
    DO SO ON AN "AS IS" BASIS, WITHOUT WARRANTY OF ANY KIND, EITHER
    EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
    PURPOSE. THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE
    SOFTWARE FALLS UPON THE USER OF THE SOFTWARE. SHOULD THE SOFTWARE
    PROVE DEFECTIVE, YOU (AS THE USER OR REDISTRIBUTOR) ASSUME ALL COSTS
    OF ALL NECESSARY SERVICING, REPAIR OR CORRECTIONS.

    IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING
    WILL DARTMOUTH COLLEGE OR ANY OTHER COPYRIGHT HOLDER, OR ANY OTHER
    PARTY WHO MAY MODIFY AND/OR REDISTRIBUTE THE SOFTWARE AS PERMITTED
    ABOVE, BE LIABLE TO YOU FOR DAMAGES, INCLUDING ANY GENERAL, SPECIAL,
    INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR
    INABILITY TO USE THE SOFTWARE (INCLUDING BUT NOT LIMITED TO LOSS OF
    DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY YOU OR
    THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER
    PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGES.

    The complete text of the license may be found in the file COPYING
    which should have been distributed with this software. The GNU
    General Public License may be obtained at
    http://www.gnu.org/licenses/gpl.html

  Project: Katana
  Date: February 2010
  Description: Functions for dealing with filepaths
*/

#include <string.h>
#include <stdbool.h>
#include "util.h"
#include <sys/stat.h>
#include "logging.h"
#include "path.h"
#include <limits.h>
#include <unistd.h>

char* joinPaths(char* path1,char* path2)
{
  if(!path1 && !path2)
  {return NULL;}
  char* result=zmalloc(strlen(path1)+strlen(path2)+2);
  if(path1)
  {
    strcpy(result,path1);
  }
  if(!path2)
  {
    return result;
  }
  if(path1 && path1[0]!='\0' && !strEndsWith(path1,"/"))
  {
    strcat(result,"/");
  }
  if(path2)
  {
    strcat(result,path2);
  }
  return result;
}

//path is made relative to ref
char* makePathRelativeTo(char* path,char* ref)
{
  char* path_=absPath(path);
  char* ref_=absPath(ref);
  if(!path_ || !ref_)
  {
    return NULL;
  }
  if(!strncmp(path_,ref_,strlen(ref_)))
  {
    char* result=strdup(path_+strlen(ref_));
    free(path_);
    free(ref_);
    return result;
  }
  else
  {
    death("Need to implement more compilated forms of makePathRelativeTo. Poke James\n");
    return NULL;
  }
}

//return the directory portion of path
//the result should be freed
char* getDirectoryOfPath(char* path)
{
  struct stat s;
  if(0!=stat(path,&s))
  {
    logprintf(ELL_WARN,ELS_PATH,"path %s does not exist, cannot determine the directory portion\n",path);
  }
  if(S_ISDIR(s.st_mode))
  {
    return strdup(path);//the whole thing is the directory portion
  }
  else
  {
    char* lastSlash=strrchr(path,'/');
    if(!lastSlash)
    {
      return strdup("");
    }
    char* result=strdup(path);
    result[lastSlash-path+1]='\0';
    return result;
  }
}

//The returned string should be freed
char* absPath(char* path)
{
  char* result=realpath(path,NULL);
  if(!result)
  {
    perror("realpath failed in absPath");
    logprintf(ELL_WARN,ELS_PATH,"realpath failed in absPath(%s)\n",path);
    return NULL;
  }
  return result;
}

bool isAbsPath(char* path)
{
  return path[0]=='/';
}
