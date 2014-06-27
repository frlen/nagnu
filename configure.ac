AC_INIT(nagnu, version-0.1)
AM_INIT_AUTOMAKE([1.10 -Wall no-define foreign])
AC_PROG_CC
AC_LANG(C)
AC_CHECK_HEADERS(stdio.h)
AC_CHECK_HEADERS(string.h)
AC_CHECK_HEADERS(sys/types.h)
AC_CHECK_HEADERS(fcntl.h)
AC_CHECK_HEADERS(stdlib.h)
AC_CHECK_HEADERS(curl/curl.h)
AC_CHECK_HEADERS(regex.h)
AC_CHECK_HEADERS(unistd.h)
AC_CHECK_HEADERS(curses.h)
AC_ARG_WITH([libcurl-include-path],
  [AS_HELP_STRING([--with-libcurl-include-path],
    [location of the libcurl headers, defaults to /usr/include])],
  [LIBCURL_CFLAGS="-I$withval"],
  [LIBCURL_CFLAGS='-I/usr/include'])
AC_SUBST([LIBCURL_CFLAGS])

AC_ARG_WITH([libcurl-lib-path],
  [AS_HELP_STRING([--with-libcurl-lib-path], [location of the libcurl libraries])],
  [LIBCURL_LIBS="-L$withval -llibcurl"],
  [LIBCURL_LIBS='-llibcurl'])
AC_SUBST([LIBCURL_LIBS]) 
AC_CHECK_LIB(curl, curl_easy_init)
AC_CHECK_LIB(ncurses, initscr)
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
