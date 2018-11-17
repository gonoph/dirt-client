# process this with automake to create the Makefile.in
bin_PROGRAMS = dirt

AM_CFLAGS   = -Wall -pipe -Wno-write-strings -Wno-deprecated -Wno-narrowing
AM_CPPFLAGS = -Ih -DLIBEXECDIR='"$(libexecdir)"' -DPREFIX='"$(prefix)"'
AM_CXXFLAGS = $(AM_CFLAGS)
AM_LDFLAGS  = 

# this is needed so that the shared library plugins can see the symbols of the
# main program 
dirt_LDFLAGS = -rdynamic

if DEBUG
AM_CFLAGS += -ggdb -O0
AM_CXXFLAGS += -ggdb -O0
endif

if EFENCE
AM_LDFLAGS += -lefence
endif

dirt_SOURCES = Buffer.cc Chat.cc Config.cc Curses.cc \
	EmbeddedInterpreter.cc GlobalStats.cc Hook.cc Hotkey.cc \
	InputBox.cc InputLine.cc Interpreter.cc MUD.cc \
	MessageWindow.cc Option.cc OutputWindow.cc Pipe.cc \
	Screen.cc Selectable.cc Selection.cc Session.cc Shell.cc \
	Socket.cc StaticBuffer.cc StatusLine.cc String.cc TTY.cc \
	Variables.cc Window.cc WindowGroup.cc main.cc misc.cc \
	h/Bitvalues.h h/Buffer.h h/Chat.h h/Color.h h/Config.h \
	h/Control.h h/Curses.h h/EmbeddedInterpreter.h h/GlobalStats.h \
	h/Hook.h h/Hotkey.h h/InputBox.h h/InputLine.h \
	h/Interpreter.h h/Label.h h/List.h h/MUD.h h/MessageBox.h \
	h/MessageWindow.h h/NullEmbeddedInterpreter.h h/Numbered.h \
	h/Option.h h/OutputWindow.h h/Pipe.h h/Plugin.h h/ProxyWindow.h \
	h/Screen.h h/ScrollbackSearch.h h/Selectable.h \
	h/Selection.h h/Session.h h/Shell.h h/Socket.h \
	h/StaticBuffer.h h/StatusLine.h h/String.h h/TTY.h \
	h/Variables.h h/Window.h h/WindowGroup.h h/config.h.in \
	h/defs.h.in h/dirt.h h/global.h h/mccpDecompress.h h/misc.h \
	h/config.h h/defs.h mccpDecompress.c

pkglib_LTLIBRARIES =
noinst_LTLIBRARIES =
if WITH_PERL
pkglib_LTLIBRARIES += perl.la
perl_la_SOURCES = plugins/PerlEmbeddedInterpreter.cc h/PerlEmbeddedInterpreter.h
perl_la_CFLAGS = @PERL_CFLAGS@ $(AM_CFLAGS)
perl_la_CXXFLAGS = @PERL_CFLAGS@ $(AM_CFLAGS)
perl_la_LDFLAGS = -module @PERL_LDFLAGS@
endif

if WITH_PYTHON
# noinst_LTLIBRARIES += python.la
pkglib_LTLIBRARIES += python.la
python_la_SOURCES = plugins/PythonEmbeddedInterpreter.cc h/PythonEmbeddedInterpreter.h
python_la_CFLAGS = @PYTHON_CFLAGS@ $(AM_CFLAGS)
python_la_CXXFLAGS = @PYTHON_CFLAGS@ $(AM_CFLAGS)
python_la_LDFLAGS = -module @PYTHON_LDFLAGS@
endif

