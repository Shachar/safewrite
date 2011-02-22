/*
   testwrite.c - test suite for the safewrite library

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

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "safewrite.h"

int main( int argc, char *argv[] )
{
    int fd;
    size_t num, len;
    void *safewrite_context;

    if( argc<4 ) {
        printf("Usage: testwrite filename str num\n"
                "  Will create file <filename> safetly, containing <num> times <str>\n");

        return 1;
    }

    num=strtoul( argv[3], NULL, 10 );

    len=strlen( argv[2] );
    if( num==0 || len==0 ) {
        fprintf( stderr, "safewrite: cannot create an empty file\n" );

        return 1;
    }

    fd=safe_open( argv[1], O_WRONLY, 0666, &safewrite_context );
    if( fd<0 ) {
        perror( "safewrite: Failed to open file" );

        return 2;
    }

    char *string=malloc( strlen(argv[2])+2 );
    if( string==NULL ) {
        perror("memory allocation failure");

        return 2;
    }

    sprintf( string, "%s\n", argv[2] );
    while( num>0 ) {
        write( fd, string, len+1 );
        num--;
    }

    if( safe_close_sync( fd, &safewrite_context )<0 ) {
        perror( "safewrite: Failure while closing the file" );

        return 2;
    }

    return 0;
}
