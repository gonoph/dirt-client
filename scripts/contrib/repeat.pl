# Repeat the last command entered when just enter is pressed

package Repeat;
my($LastCommandEntered) = "";
my($repeatcmdstart) = &main::str_to_color("bold_Blue_White");

sub check_repeat {
    if ($_ eq "") {
        $_ = $LastCommandEntered;
        return 1;
    } else {
        $LastCommandEntered = $_;
        return 0;
    }
}

#&hook_add("userinput", "check_repeat", \&check_repeat);
&main::run("/hook -T USERINPUT __DIRT_REPEAT_check = /run -Lperl Repeat::check_repeat");

print "Loaded auto/repeat.pl\t(Will cause <enter> on blank prompt to send last command)\n";

1;
