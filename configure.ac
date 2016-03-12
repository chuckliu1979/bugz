#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

#
m4_include([m4/version.m4])

#
AC_PREREQ([2.59])
AC_INIT([bugz], [BUGZ_VERSION], [19246678@qq.com])
AM_INIT_AUTOMAKE([bugz],[BUGZ_VERSION])
AC_CONFIG_SRCDIR([src/bugz.h])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_MACRO_DIR([m4])

#
PACKAGE_MAJOR="BUGZ_MAJOR"
PACKAGE_MINOR="BUGZ_MINOR"
PACKAGE_PATCH="BUGZ_PATCH"
AC_SUBST(PACKAGE_MAJOR)
AC_SUBST(PACKAGE_MINOR)
AC_SUBST(PACKAGE_PATCH)

#
AC_DEFINE_UNQUOTED([PACKAGE_MAJOR],[$PACKAGE_MAJOR],[Define to the major])
AC_DEFINE_UNQUOTED([PACKAGE_MINOR],[$PACKAGE_MINOR],[Define to the minor])
AC_DEFINE_UNQUOTED([PACKAGE_PATCH],[$PACKAGE_PATCH],[Define to the patch])

# Checks for programs.
AC_LANG([C])
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([magic],[magic_file],,AC_MSG_ERROR([unable to find libmagic]))
AC_CHECK_LIB([curl],[curl_version_info],,AC_MSG_ERROR([unable to find libcurl]))
AC_CHECK_LIB([json-c],[json_c_version],,AC_MSG_ERROR([unable to find libjson-c]))

# Checks for header files.
AC_CHECK_HEADERS([magic.h])
AC_CHECK_HEADERS([curl/curl.h])
AC_CHECK_HEADERS([json-c/json.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

#
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([doc/Makefile])
AC_CONFIG_FILES([man/Makefile])
AC_CONFIG_FILES([examples/Makefile])
#AC_CONFIG_FILES([scripts/Makefile])

#
AC_OUTPUT
