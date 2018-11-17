# process this with automake to create the Makefile.in

ACLOCAL_AMFLAGS = -I m4

# Refresh configuration if libtool dependencies change
LIBTOOL_DEPS = @LIBTOOL_DEPS@
libtool: $(LIBTOOL_DEPS)
	$(SHELL) $(top_builddir)/config.status libtool

# SUBDIRS = src tests
SUBDIRS = src

dist_doc_DATA = AUTHORS BUGS COPYING ChangeLog INSTALL NEWS Notes
dist_doc_DATA += README README.1ST README.md TODO dirt2.spec

EXTRA_DIST = doc libtool m4 scripts autogen.sh
