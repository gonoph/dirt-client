# Player scripts
#
# The purpose of these triggers/scripts is to keep information about they player's
# character.  Then this information is available to other scripts.  This is
# basically a set of state variables about the player.  Hitpoints, spell points,
# whether the player is meditating, sleeping, fighting, how many hit points (s)he
# has, etc.
#
# This is clearly *very* mud dependent, and is intended as an example of what
# could be done.

%Player = ();  # All info will be kept in this global hash.

sub player_triggers {
    $uncolored = $_;
    $uncolored =~ s/\xEA.//g;

    # boolean: $Player{'meditating'}
    if($uncolored =~ /^You stop meditating, feeling fully refreshed\.$/) {
        $Player{'meditating'} = 0;
    }
    elsif($uncolored =~ /^You sit down and close your eyes to begin meditating\.$/) {
        $Player{'meditating'} = 1;
    }
    elsif($uncolored =~ /^You stop meditating\.$/) {
    }

    # HP bar (Breed guild, Three Kingdoms mud at 3k.org:5000):
    if($uncolored =~ m#^HP: ([0-9]+)/([0-9]+) SP: ([0-9]+)/([0-9]+)  Psi: ([0-9]+)/([0-9]+)  Focus: ([0-9]+)%  E: ([A-Z][a-z]+)$#) {
        $Player{'hp'} = $1;
        $Player{'maxhp'} = $2;
        $Player{'sp'} = $3;
        $Player{'maxsp'} = $4;
        $Player{'psi'} = $5;
        $Player{'maxpsi'} = $6;
        $Player{'focus'} = $7;
        $Player{'enemy'} = $8;
    }

    # Coins -- output of the 'coins' command
    if($uncolored =~ /^You are carrying ([0-9]+) coins in loose change\.$/) {
        $Player{'coins'} = $1;
    } 
    elsif($uncolored =~ /^You have ([0-9]+) coins in bags\.$/) {
        $Player{'coins in bags'} = $1;
    }
    elsif($uncolored =~ /^You have ([0-9]+) coins in the bank\.$/) {
        $Player{'coins in bank'} = $1;
    }
}

# add the above function to Dirt's hook list.  The function will be called with
# no parameters, and $_ set to the line received from the mud.
&main::hook_add("/hook -T OUTPUT -f player_triggers = player_triggers");
