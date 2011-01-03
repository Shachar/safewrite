ACLOCAL_AMFLAGS=-I m4

lib_LTLIBRARIES = libsafewrite.la
noinst_PROGRAMS = testwrite
include_HEADERS = safewrite.h

libsafewrite_la_SOURCES = safewrite.c
libsafewrite_la_LDFLAGS = -version-info 0:0:0

testwrite_SOURCES = testwrite.c
testwrite_LDADD = libsafewrite.la

man3_MANS = safe_open.man safe_close.man safe_close_sync.man
