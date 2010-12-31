/*
   safewrite.c - race free file update helper functions.

   Copyright (c) 2010 Lingnu Open Source Consulting Ltd. http://www.lingnu.com

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
 */

#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "safewrite.h"

int safe_open( char buffer[PATH_MAX], int flags, mode_t mode )
{
    int fd;
    char newname[PATH_MAX];
    struct stat state;
    ssize_t sres; // Signed results

    /*
       Open the old file with the desired mode. This serves two purposes:
       A. It makes sure that the user actually left us enough permissions to
          open the file with the desired access.
       B. It offloads the job of making sure there is no symbolic link loop to
          the kernel.
     */
    flags &= ~(O_CREAT|O_TRUNC); // Do not create and do not truncate the old file.
    fd=open( buffer, flags );
    if( fd<0 && errno!=ENOENT )
        // Couldn't open a file for a reason other than it doesn't exist
        return -1;

    // Resolve the symbolic links
    while( (sres=readlink( buffer, buffer, PATH_MAX-1 ))>=0 ) {
        // Make sure we're NULL terminated. Wash, rinse, repeat.
        buffer[sres]='\0';
    }

    // The only legitimate reason for readlink to fail is if it does not point
    // at a symlink, which results in errno=EINVAL.
    if( errno!=EINVAL ) {
        close(fd);

        return -1;
    }

    // Make sure the buffer is big enough
    if( strlen(buffer)>PATH_MAX-5 ) {
        errno=ENAMETOOLONG;
        close(fd);

        return -1;
    }

    // Create the temporary file's name
    strcpy( newname, buffer );
    strcat( newname, ".tmp" );

    if( unlink( newname )<0 && errno!=ENOENT ) {
        // If the unlink failed, we won't be able to create the new file anyways
        close(fd);

        return -1;
    }

    // Get the information about the existing file, if any
    if( fd>=0 ) {
        fstat( fd, &state );

        close(fd);

        // Open the file, creating if didn't already exist.
        fd=open( newname, flags|O_CREAT|O_TRUNC, 0600 );

        if( fd>=0 ) {
            // The following two may fail for lack of permission, but we ignore this failure
            fchown( fd, state.st_uid, -1 ); 
            fchown( fd, -1, state.st_gid ); 
            fchmod( fd, state.st_mode&07777 );
        }
    } else {
        // No existing config file - create a new one with the desired mode
        fd=open( newname, flags|O_CREAT|O_TRUNC, mode );
    }

    return fd;
}

int safe_close( const char buffer[PATH_MAX], int fd )
{
    char newname[PATH_MAX];

    close(fd);

    // Trust the user not to change buffer since the call to safe_open
    strcpy( newname, buffer );
    strcat( newname, ".tmp" );

    return rename( newname, buffer );
}
