# Gags package
package Gag;

use vars qw(%Gags);  # Variables that will be exported in Gag:: namespace
if(!defined %Gags) {
    %Gags = ();
}
&main::save("Gag::Gags", \%Gags);


# command '/gag' (list/add/delete gags)
sub main::dirtcmd_gag {
    my ($count) = (0);
    if (/^(\d+)/) {
        # Remove a certain gag
        foreach (keys %Gags) {
            if (++$count == $1) {
                print "Deleted gag: $_\n";
                delete $Gags{$_};
                return;
            }
        }
        print "No such gag (there are only $count!) - type gag to see a list.\n";
    } elsif (/^.{3,}$/) {
        $Gags{$_} = 0;
        print "Gagging '$_' from now on.\n" unless $Loading;
    } else {
        print "Following gags are active:\n";
        foreach (keys %Gags) {
            printf "%2d) $_ (%d)\n", ++$count, $Gags{$_};
        }
        print "\nUse gag <string> to add a gag.\n";
        print "Use gag <number> to remove.\n";
    }
}

# Run though gags and apply any that are seen
sub gag_input_hook {
    foreach $gag (keys %Gags) {
        if (/$gag/) {
            $Gags{$gag}++;
            $_ = "";
            return 1;
        }
    }
    return 0;
}

# save gags to config file.
#sub save_gags {
#    local (*FH) = $_[0];
#    foreach (keys %Gags) {
#        print FH "Gag $_\n";
#    }
#}

#&main::hook_add("output", "gag_input_hook", \&gag_input_hook, 0, 0);
&main::run($main::commandCharacter . "hook -T OUTPUT -F -fL perl gag_input = Gag::gag_input_hook");
#save_add(\&save_gags);
#load_add("gag", \&dirtcmd_gag);

print "Loaded auto/gag.pl\t(Routines to gag mud output)\n";

1;
