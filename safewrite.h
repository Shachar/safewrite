#ifndef SAFEWRITE_H
#define SAFEWRITE_H

/**
 *
 */
int safe_open( char buffer[PATH_MAX], int flags, mode_t mode );
int safe_close( const char buffer[PATH_MAX], int fd );

#endif /* SAFEWRITE_H */
