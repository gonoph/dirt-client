# process this with automake to create the Makefile.in

ACLOCAL_AMFLAGS = -I m4

# Refresh configuration if libtool dependencies change
LIBTOOL_DEPS = @LIBTOOL_DEPS@
libtool: $(LIBTOOL_DEPS)
	$(SHELL) $(top_builddir)/config.status libtool

# SUBDIRS = src tests
SUBDIRS = src

dist_doc_DATA = AUTHORS BUGS COPYING ChangeLog INSTALL NEWS Notes README \
	README.1ST README.md TODO dirt.spec 

dirt_docs_dir = $(docdir)/doc
dirt_docs_commands_dir = $(dirt_docs_dir)/commands
dirt_docs_old_dir = $(dirt_docs_dir)/old
dirt_docs__DATA = doc/CommandLine doc/Escaping doc/Help doc/Hooks doc/Keys doc/Scripts doc/Trigger

dirt_docs_commands__DATA = doc/commands/alias doc/commands/bell \
	doc/commands/clear doc/commands/close doc/commands/command \
	doc/commands/complete doc/commands/disable doc/commands/echo \
	doc/commands/enable doc/commands/eval doc/commands/exec doc/commands/gag \
	doc/commands/group doc/commands/hook doc/commands/keypress doc/commands/load \
	doc/commands/open doc/commands/prompt doc/commands/quit doc/commands/reopen \
	doc/commands/run doc/commands/save doc/commands/send doc/commands/setinput \
	doc/commands/status doc/commands/trig doc/commands/trigger doc/commands/window \
	doc/commands/windows

dirt_docs_old__DATA = doc/old/Changes doc/old/Chat doc/old/Chat.html \
	doc/old/Embedded doc/old/Embedded.html doc/old/Examples doc/old/Examples.html \
	doc/old/Modules doc/old/Modules.html doc/old/Plugins doc/old/README \
	doc/old/README.html doc/old/TODO 

dirt_dir = $(libexecdir)/@PACKAGE_NAME@
dirt_scripts_dir = $(dirt_dir)/scripts
dirt_scripts_3k_dir = $(dirt_scripts_dir)/3k
dirt_scripts_auto_dir = $(dirt_scripts_dir)/auto
dirt_scripts_contrib_dir = $(dirt_scripts_dir)/contrib
dirt_scripts_sys_dir = $(dirt_scripts_dir)/sys

dirt_scripts_3k__DATA = scripts/3k/Player.pl scripts/3k/Saved.pl \
	scripts/3k/ThreeKingdoms.pl scripts/3k/aliases.pl

dirt_scripts_auto__DATA = scripts/auto/README scripts/auto/completion.pl \
	scripts/auto/help.pl scripts/auto/mudftp.pl scripts/auto/alias.pl \
	scripts/auto/command.pl scripts/auto/gag.pl scripts/auto/highlight.pl \
	scripts/auto/keypress.pl scripts/auto/trigger.pl

dirt_scripts_contrib__DATA = scripts/contrib/DynaComplete.py \
	scripts/contrib/README scripts/contrib/bot.pl scripts/contrib/bot.py \
	scripts/contrib/ccancel.pl scripts/contrib/log.pl \
	scripts/contrib/mudcompletion.pl scripts/contrib/profanity.pl \
	scripts/contrib/prompt.pl scripts/contrib/reconnect.pl \
	scripts/contrib/repeat.pl scripts/contrib/spellcheck.pl scripts/contrib/vars.pl

dirt_scripts_sys__DATA = scripts/sys/color.pl scripts/sys/config.pl \
	scripts/sys/config.py scripts/sys/functions.pl scripts/sys/functions.py \
	scripts/sys/idle.pl scripts/sys/idle.py scripts/sys/init.py scripts/sys/keys.pl \
	scripts/sys/init.pl

EXTRA_DIST = libtool m4 scripts autogen.sh doc
