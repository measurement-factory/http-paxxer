lib_LTLIBRARIES = \
	libHttpPaxxer.la

libHttpPaxxer_la_SOURCES = \
	All.cc

include_pkg_HEADERS = \
	All.h \
	Connection.h \
	Forward.h \
	Paxxer.h \
	Stream.h

nodist_include_pkg_HEADERS = \
        $(top_builddir)/src/HttpPaxxer/autoconf.h

include_pkgdir = ${prefix}/include/@PACKAGE@

DISTCLEANFILES = \
	ac-autoconf.h \
	$(nodist_include_pkg_HEADERS)

# top_builddir/src/ is needed for the generated autoconf.h
AM_CPPFLAGS = \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src

libHttpPaxxer_la_LDFLAGS = -version-info $(HTTP_PAXXER_LIBRARY_VERSION)
