# This utility function needs be located in this file
# Use this, not require directly!

use Carp qw(carp cluck croak confess); # verbose warning/error messages.
use IO::Handle;

open(LOG, ">/tmp/dirtlog") || die "unable to open dirtlog\n";
LOG->autoflush(1);

# Fallback to global installation
push @INC, "$ENV{HOME}/.dirt", "/usr/local/lib/dirt", "/usr/lib/dirt";

# mbt aug 00 -- hack to make things work under Debian (ugh)
if (-f "/etc/debian_version") {
  push @INC, "/usr/lib/perl5/5.005", "/usr/lib/perl5/5.005/i386-linux";
}
require "sys/functions.pl"; # Lots of utility functions

# FIXME make this an INIT hook instead?
&run($commandCharacter . "hook -T INIT -fL perl perl_init = init");
sub init {
#    require "sys/functions.pl"; # Lots of utility functions
    require "sys/color.pl";     # Color code definitions
    require "sys/idle.pl";      # Callouts
    require "sys/config.pl";    # Configuration management
    require "sys/keys.pl";      # $keySomething definitions
    
    if(-f "$DIRT_HOME/lib/dirt/localinit.pl") {
      require "localinit.pl";
   }
    
    # Do autoloading. Just use builtin glob, to reduce dependency
    # on Perl version
    my @AutoloadDirectories = ();
    if (-d "$ENV{HOME}/.dirt/auto") {
        push @AutoloadDirectories, "$ENV{HOME}/.dirt/auto";
    } else {
        push @AutoloadDirectories, "/usr/local/lib/dirt/auto", "/usr/lib/dirt/auto";
    }
    
    foreach $AutoDir (@AutoloadDirectories) {
        foreach (glob("$AutoDir/*.pl")) {
            include $_;
            #require $_;
        }
        
        foreach (glob("$AutoDir/*")) {
            if (-d $_ and /\/([^\/]+)$/ and -f "$_/$1.pl") {
                require("$_/$1.pl");
            }
        }
    }

#    &load_configuration(); # This is now installed as an INIT hook -- see config.pl
    &run($commandCharacter . "hook -d perl_init");  # Delete myself from init list
    &run($commandCharacter . "hook -T INIT -r ''"); # and run INIT again in case require'ed scripts need it.
}

print "Loaded init.pl\t\t(Loads everything else)\n";
1;
