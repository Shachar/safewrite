/*
   safewrite.c - race free file update helper functions.

   Copyright (c) 2010,2011 Lingnu Open Source Consulting Ltd. http://www.lingnu.com

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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

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

    // Before doing anything else, resolve all symbolic links
    if( realpath( buffer, newname )==NULL ) {
        char *filepart;

        // Did we fail for file not found?
        if( errno!=ENOENT )
            return -1;
        
        // The file doesn't exist, try to resolve just the directory name.
        filepart=strrchr( buffer, '/' );
        if( filepart!=NULL ) {
            // split the path into dir part and file part
            *(filepart++)='\0';

            if( realpath( buffer, newname )==NULL ) {
                // Still can't resolve the path - lose all hope, curl in a corner and die
                return -1;
            }
        } else {
            // the original string did not contain any slashes, use the current dir as the "dir part" and "buffer" as
            // the file part
            filepart=buffer;

            if( realpath( ".", newname )==NULL )
                // Can't tell the full path of the directory we are actually in. Give up.
                return -1;
        }

        // Do we have enough space for the combined string (dir name + file part + / + nul)?
        if( strlen( newname ) + strlen( filepart ) > PATH_MAX-2 ) {
            errno=ENAMETOOLONG;

            return -1;
        }

        strcat( newname, "/" );
        strcat( newname, filepart );

        /*
           XXX At this point, if "filepart" is a symbolic link to a non-existing file, ideally, we would have liked for
           the symbolic link destination to be created. What will happen here is that, instead, the symbolic link itself
           will be replaced.

           While, as mentioned, not ideal, the alternative is to re-implement "realpath" so that it does not fail if the
           last component of the path does not exist.
         */
    }

    // Make sure the buffer is big enough after the suffixes we need to add
    if( strlen(newname)>PATH_MAX-5 ) {
        errno=ENAMETOOLONG;

        return -1;
    }

    // Update the original buffer and create the temporary file's name
    strcpy( buffer, newname );
    strcat( newname, ".tmp" );

    /*
       At this point "buffer" is an absolute path to the actual file we want to replace. We no longer care about the
       original path, only this new "real" one.

       Open the old file with the desired mode. This makes sure that the user actually left us enough permissions to
       open the file with the desired access.
     */
    flags &= ~(O_CREAT|O_TRUNC); // Do not create and do not truncate the old file.
    fd=open( buffer, flags );
    if( fd<0 && errno!=ENOENT )
        // Couldn't open a file for a reason other than it doesn't exist
        return -1;

    if( unlink( newname )<0 && errno!=ENOENT ) {
        // If the unlink failed, we won't be able to create the new file anyways
        close(fd);

        return -1;
    }

    // Get the information about the existing file, if any
    if( fd>=0 ) {
        // Old file exists
        struct stat state;

        fstat( fd, &state );

        close(fd);

        // Open the file, creating if didn't already exist.
        fd=open( newname, flags|O_CREAT|O_TRUNC|O_NOFOLLOW, 0600 ); // Give very few permissions to avoid a race

        if( fd>=0 ) {
            /*
               The fchowns may fail for lack of permission, but we ignore this failure as our only commitment is best
               effort. We set the UID and the GID in separate system calls, as it is possible we have enough permissions
               to do one but not the other.

               We do not copy the SUID and SGID flags unless the respective chown succeeded.
             */
            mode_t mask=01777;
            if( fchown( fd, state.st_uid, -1 )==0 )
                mask|=04000;
            if( fchown( fd, -1, state.st_gid )==0 )
                mask|=02000;

            fchmod( fd, state.st_mode&mask );
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

    // Make sure the data is actually on disk
    fsync( fd ); // XXX Should we check for error?

    if( close(fd)<0 )
        return -1;

    // Trust the user not to change buffer since the call to safe_open, so no need to repeat the checks
    strcpy( newname, buffer );
    strcat( newname, ".tmp" );

    return rename( newname, buffer );
}
