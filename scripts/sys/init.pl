# Be careful with errors in this file.  If loading this file fails, Carp will
# not be loaded and you will not see perl errors in dirt!

use Carp qw(carp cluck croak confess); # verbose warning/error messages.
use IO::Handle;
require DynaLoader;

require "sys/functions.pl"; # Lots of utility functions
&run($commandCharacter . "hook -T INIT perl_init = ${commandCharacter}run -Lperl init");
sub init {
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
    
    foreach (@INC) {
        my $d = "$_/auto";
        $d =~ s|//|/|g;
        if (-d $d) {
            if (glob("$d/*.pl") or glob("$d/*/*.pl")) {
                print "Added $d\n";
                push @AutoloadDirectories, $d;
            }
        }
    }
    
    foreach my $AutoDir (@AutoloadDirectories) {
        my @files = glob("$AutoDir/*.pl");
        push @files, glob("$AutoDir/*/*.pl");
        if (@files) {
            printf "Processing %s\n", $AutoDir;
        }
        foreach (@files) {
            # print "Loading ", $_, "\n";
            do $_;
            #require $_;
        }
    }

#    &load_configuration(); # This is now installed as an INIT hook -- see config.pl
    &run($commandCharacter . "hook -d perl_init");  # Delete myself from init list
    &run($commandCharacter . "hook -T INIT -r ''"); # and run INIT again in case require'ed scripts need it.
}

print "Loaded sys/init.pl\t(Loads everything else)\n";
1;
