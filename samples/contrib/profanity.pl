#  Profanity filter

# Words to replace
@Profanity = qw/shit fuck/;

sub check_profanity {
    my $p;

    # Replace with a string of * of equal length
    foreach $p (@Profanity) {
        s/$p/'*' x length($p)/gei;
    }
}

&output_add(\&check_profanity);

# Optionally, filter your OWN output before sending it to the MUD!
&userinput_add(\&check_profanity);

