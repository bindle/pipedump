#
#   Pipe Dump Utility & Library
#   Copyright (C) 2012 Bindle Binaries <syzdek@bindlebinaries.com>.
#
#   @BINDLE_BINARIES_BSD_LICENSE_START@
#
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions are
#   met:
#
#      * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#      * Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in the
#        documentation and/or other materials provided with the distribution.
#      * Neither the name of Bindle Binaries nor the
#        names of its contributors may be used to endorse or promote products
#        derived from this software without specific prior written permission.
#
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
#   IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
#   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
#   PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL BINDLE BINARIES BE LIABLE FOR
#   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
#   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
#   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
#   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
#   OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
#   SUCH DAMAGE.
#
#   @BINDLE_BINARIES_BSD_LICENSE_END@
#
#   @configure_input@
#
#   Makefile.am - automate compiling on a unix platform
#

# Sub directories
SUBDIRS					=
DIST_SUBDIRS				=

# Global flags
AM_LIBS					=
AM_LDFLAGS				=
AM_CFLAGS				= -O2 $(CFLAGS_WARNINGS)
AM_CXXFLAGS				= -O2 @AM_CXXFLAGS@
AM_OBJCFLAGS				= -O2 @AM_OBJCFLAGS@
AM_CPPFLAGS				= -O2 -UPMARK -DHAVE_CONFIG_H=1 -I$(top_builddir)/include -I$(top_srcdir)/include -DSYSCONFDIR="\"$(sysconfdir)\""
ACLOCAL_AMFLAGS				= -I m4 -I contrib/bindletools/m4 -W all -W error
AM_MAKEINFOFLAGS                        = --no-split
DEFS					=


# automake targets
check_PROGRAMS				=
doc_DATA				=
include_HEADERS				= include/pipedump.h
lib_LTLIBRARIES				= lib/libpipedump.la
man_MANS				= doc/pipedump.1
info_TEXINFOS				=
noinst_LTLIBRARIES			=
noinst_HEADERS				=
noinst_PROGRAMS				=
bin_PROGRAMS				= src/pipedump
bin_SCRIPTS				=
sbin_SCRIPTS				=


# macros for lib/libotadm.la
lib_libpipedump_la_DEPENDENCIES		= Makefile lib/libpipedump.sym
lib_libpipedump_la_CFLAGS		= $(AM_CFLAGS)
lib_libpipedump_la_LIBADD		= $(AM_LIBS)
lib_libpipedump_la_LDFLAGS		= -rpath '$(libdir)' \
					  -version-info $(LIB_VERSION_INFO) \
					  $(AM_LDFLAGS)
					  -export-symbols $(srcdir)/lib/libpipedump.sym
lib_libpipedump_la_SOURCES		= lib/libpipedump.c \
					  lib/libpipedump.h


# macros for src/pipedump
src_pipedump_DEPENDENCIES		= $(lib_LTLIBRARIES) Makefile
src_pipedump_CPPFLAGS			= -DPROGRAM_NAME="\"pipedump\"" $(AM_CPPFLAGS)
src_pipedump_LDADD			= $(AM_LDADD) lib/libpipedump.la
src_pipedump_SOURCES			= $(noinst_HEADERS) \
					  src/pipedump.c


# substitution routine
do_subst = sed \
	-e 's,[@]SHELL[@],@SHELL@,g' \
	-e 's,[@]PACKAGE_VERSION[@],@PACKAGE_VERSION@,g' \
	-e 's![@]PACKAGE_NAME[@]!@PACKAGE_NAME@!g' \
	-e 's,[@]PACKAGE_TARNAME[@],@PACKAGE_TARNAME@,g' \
	-e 's,[@]PACKAGE_BUGREPORT[@],@PACKAGE_BUGREPORT@,g' \
	-e 's,[@]RELEASE_MONTH[@],@RELEASE_MONTH@,g' \
	-e 's,[@]INSTALL[@],$(INSTALL),g' \
	-e 's,[@]INSTALL_PROGRAM[@],$(INSTALL_PROGRAM),g' \
	-e 's,[@]INSTALL_SCRIPT[@],$(INSTALL_SCRIPT),g' \
	-e 's,[@]INSTALL_DATA[@],$(INSTALL_DATA),g' \
	-e 's,[@]LIB_VERSION_CURRENT[@],@LIB_VERSION_CURRENT@,g' \
	-e 's,[@]LIB_VERSION_REVISION[@],@LIB_VERSION_REVISION@,g' \
	-e 's,[@]LIB_VERSION_AGE[@],@LIB_VERSION_AGE@,g' \
	-e 's,[@]LIB_VERSION_INFO[@],@LIB_VERSION_INFO@,g' \
	-e 's,[@]LIB_RELEASE_INFO[@],@LIB_RELEASE_INFO@,g' \
	-e 's,[@]prefix[@],$(prefix),g' \
	-e 's,[@]datadir[@],$(datadir),g' \
	-e 's,[@]sbindir[@],$(sbindir),g' \
	-e 's,[@]sysconfdir[@],$(sysconfdir),g' \
	-e 's,[@]localstatedir[@],$(localstatedir),g'
do_subst_fn = \
		echo "do_subst < ${@}.in > ${@}"; \
		mkdir -p `dirname ${@}` || exit 1; \
		${do_subst} < $(srcdir)/${@}.in > ${@} || exit 1; \
		chmod 0755 ${@}
do_subst_dt = \
		echo "do_subst < ${@}.in > ${@}"; \
		mkdir -p `dirname ${@}` || exit 1; \
		${do_subst} < $(srcdir)/${@}.in > ${@} || exit 1; \
		chmod 0644 ${@}


# lists
BUILT_SOURCES				= include/git-package-version
TESTS					=
XFAIL_TESTS				=
EXTRA_MANS				=
EXTRA_DIST				= build-aux/git-tar-name.txt \
					  include/git-package-version.txt \
					  include/git-package-version.h \
					  contrib/bindletools/COPYING \
					  contrib/bindletools/README \
					  contrib/bindletools/build-aux/autogen.sh \
					  contrib/bindletools/build-aux/git-package-version.sh \
					  contrib/bindletools/m4/bindle.m4 \
					  doc/pipedump.1.in \
					  lib/libpipedump.sym
CLEANFILES				= \
					  $(builddir)/a.out   $(srcdir)/a.out \
					  $(builddir)/*/a.out $(srcdir)/*/a.out \
					  config.h.in~ $(srcdir)/config.h.in~ \
					  $(man_MANS) \
					  $(bin_SCRIPTS) \
					  $(sbin_SCRIPTS) \
					  @PACKAGE_TARNAME@-*.tar.* \
					  @PACKAGE_TARNAME@-*.zip
DISTCHECK_CONFIGURE_FLAGS		= --enable-strictwarnings


# custom targets
.PHONY: autoreconf update gitclean include/git-package-version

include/git-package-version:
	@@GIT_VERSION_SCRIPT@ $(top_srcdir) > /dev/null 2>&1

autoreconf:
	autoreconf -i -f $(ACLOCAL_AMFLAGS) $(top_srcdir)

doc/pipedump.1: Makefile $(srcdir)/doc/pipedump.1.in
	@$(do_subst_dt)

update:
	git --git-dir=$(top_srcdir)/.git pull 2> /dev/null || exit 0
	$(top_srcdir)/build-aux/autogen.sh
	test -x ./config.status && { ./config.status --recheck || exit $?; } || exit 0

gitclean:
	$(MAKE) -f Makefile distclean
	rm -f  $(top_srcdir)/INSTALL
	rm -f  $(top_srcdir)/Makefile.in
	rm -f  $(top_srcdir)/aclocal.m4
	rm -f  $(top_srcdir)/config.h.in
	rm -f  $(top_srcdir)/configure
	rm -f  $(top_srcdir)/m4/l*
	rm -f  $(top_srcdir)/build-aux/c* $(top_srcdir)/build-aux/d*
	rm -f  $(top_srcdir)/build-aux/i* $(top_srcdir)/build-aux/l*
	rm -f  $(top_srcdir)/build-aux/m*
	rm -f  $(top_srcdir)/include/git-package-version*


# local targets
install-exec-local:

install-data-local:

uninstall-local:

clean-local:

distclean-local:
	rm -fR $(srcdir)/autom4te.cache

# end of automake file
