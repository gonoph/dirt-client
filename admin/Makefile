# Generated automatically from Makefile.in by configure.
# This will not build but on my machine
BASEDIR=/root/ed
VERSION := `tail -1 $(BASEDIR)/dirt/VERSION`
DIRT := dirt-$(VERSION)
DOCS := doc/README doc/Embedded doc/Modules doc/Examples doc/Chat
HTMLDOCS := $(patsubst %, %.html, $(DOCS))

ALL_SOURCE = $(SRC_FILES) $(INC_FILES)
# Common files for all distributions
_COMMON_FILES := $(DOCS) scripts VERSION doc/*.html doc/Changes doc/TODO INSTALL README.1ST doc/Plugins
_SOURCE_FILES := $(ALL_SOURCE) *.c $(INCL) Makefile.in admin/Makefile.in admin/config.* configure.in configure \
.depend o/dummy plugins/*.cc o/plugins/.dummy dirt.spec admin/install-sh \
dirt.spec.in VERSION.in h/config.h.in

_BINARY_FILES := dirt plugins/*.so

COMMON_FILES := $(patsubst %, $(DIRT)/%, $(_COMMON_FILES))
SOURCE_FILES := $(patsubst %, $(DIRT)/%, $(_SOURCE_FILES))
BINARY_FILES := $(patsubst %, $(DIRT)/%, $(_BINARY_FILES))

ftp:	distrib
	./put

%.html: %
	util/htmlize.pl $<

docs: $(HTMLDOCS)

EXCLUDE := --exclude CVS --exclude localinit.pl --exclude config --exclude dirtrc --exclude logs \
	--exclude config.perl --exclude config.python --exclude history

rpms: docs srcdistrib
	set +x; \
	cd /usr/src/redhat/SPECS ; \
	cp dirt.spec dirt-$(VERSION).spec ; \
	perl -pi -e "s/^Version.*/Version: $(VERSION)/" dirt-$(VERSION).spec ; \
	cp $(BASEDIR)/dirt/dist/dirt-$(VERSION)-src.tar.gz /usr/src/redhat/SOURCES ; \
	rpm -ba dirt-$(VERSION).spec ; \
	cd /usr/src/redhat ; \
	cp RPMS/*/dirt-$(VERSION)*rpm $(BASEDIR)/dirt/dist; \
	cp SRPMS/dirt-$(VERSION)*rpm $(BASEDIR)/dirt/dist ; \
	rm SPECS/dirt-$(VERSION).spec

autoconf:
	autoconf
	CXXFLAGS=-O ./configure --enable-debug


srcdistrib:
	rm -f dist/*.gz
	ln -s $(BASEDIR)/dirt ../$(DIRT)
	strip dirt o/plugins/*so
	ln -sf o/plugins/*so plugins/
	(cd .. ; tar cvfh $(DIRT)/dist/dirt-$(VERSION)-src.tar $(EXCLUDE) $(COMMON_FILES) $(SOURCE_FILES))
	(cd .. ; tar cvfh $(DIRT)/dist/dirt-$(VERSION)-bin.tar $(EXCLUDE) $(COMMON_FILES) $(BINARY_FILES))
	gzip -c9 dist/dirt-$(VERSION)-src.tar >  dist/dirt-$(VERSION)-src.tar.gz
	gzip -c9 dist/dirt-$(VERSION)-bin.tar >  dist/dirt-$(VERSION)-bin.tar.gz
	rm ../$(DIRT)
	rm dist/*tar

distrib:
	find scripts -name "*~" -print | xargs rm -f
	$(MAKE) autoconf
	$(MAKE) docs
	$(MAKE) commit
	$(MAKE) all
	$(MAKE) srcdistrib
#	bzip2 -c9 dist/dirt-$(VERSION)-src.tar >  dist/dirt-$(VERSION)-src.tar.bz2
#	bzip2 -c9 dist/dirt-$(VERSION)-bin.tar >  dist/dirt-$(VERSION)-bin.tar.bz2
	$(MAKE) rpms
	touch ~/html/dirt/index.sl 			# Make sure it gets rebuilt
	util/asksync.sh

commit:
	util/checkversion.sh
	util/docommit.sh
