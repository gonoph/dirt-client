# Color definitions

$ColorCode = "\xEA";

{
    my $C = "\xEA";
    $SoftCR = "\xEB";
    
    $CSave = "\xEA\xFF";
    $CRestore = "\xEA\xFE";
    
    @Colors = qw/Black Blue Green Cyan Red Magenta Yellow White/;
    
    my($x) = 0;
    foreach my $fg (@Colors) {
        $ColorVal{$Colors[$x]} = $x;
        my($lcx);
        $lcx = lc $Colors[$x];
        eval "\$$Colors[$x] = \"${C}"      . chr($x) . "\";" or print "ERROR: $@\n";
        eval "\$$lcx = \"${C}"             . chr($x)   . "\";" or print "ERROR: $@\n";
        eval "\$Bold_$Colors[$x] = \"${C}" . chr($x+8) . "\";" or print "ERROR: $@\n";
        eval "\$bold_$lcx = \"${C}"        . chr($x+8) . "\";" or print "ERROR: $@\n";
        foreach my $bg (@Colors) {
          my($lcy, $str);
          my($y) = 0;
          $lcy = lc $Colors[$y];
          eval "\$$Colors[$x]" . "_" . "$Colors[$y] = \"${C}"      . chr($x+($y<<4)) . "\";" or print "ERROR: $@\n";
          eval "\$$lcx" . "_" . "$lcy = \"${C}"                    . chr($x+($y<<4)) . "\";" or print "ERROR: $@\n";
#          $str = "\$$lcx" . "_" . "$lcy = \"${C}"                  . chr($x+($y<<4)) . "\";" or print "ERROR: $@\n";
#          $str = "\$$lcx" . "_" . "$lcy = \"" . ord(chr($x+($y<<4))) . "\";" or print "ERROR: $@\n";
#          eval $str;
#          eval "print \"this is \${${lcx}_${lcy}}${lcx}_${lcy}\${White}\"\n";
          eval "\$Bold_$Colors[$x]" . "_" . "$Colors[$y] = \"${C}" . chr($x+($y<<4)+8) . "\";" or print "ERROR: $@\n";
          eval "\$bold_" . $lcx . "_$lcy = \"${C}"             . chr($x+($y<<4)+8) . "\";" or print "ERROR: $@\n";
          $y++;
        }
        $x++;
    }

    $Off = $White_Black; # Alias
    $off = $white_black;
}

# Set both foreground and background color
sub setColor ($$) {
    my ($fg, $bg) = @_;
    return "\xEA" . chr(ord(substr($fg, 1, 1)) + 16 * ord (substr($bg, 1,1)));
}

# bold_green, bold_green_red etc.
sub str_to_color {
    my ($str,$fg,$bg) = ($_[0],0,0);
    $str =~ s/((^|_)\w)/uc $1/ge;
    if ($str =~ /^(bold_)?([^_]+)_?(.*)/i) {
        $fg = $ColorVal{$2} if exists $ColorVal{$2};
        $bg = $ColorVal{$3} if exists $ColorVal{$3};
        $fg += 8 if(defined $1 && length($1));
        return ("\xEA" . chr($fg + ($bg << 4)));
    }
    else {
        return "";
    }
}

# The other way around
sub color_to_str {
    my ($col,$val,$bold,$fg,$bg) = ($_[0], "", "", "", "");
    if (substr($col,0,1) eq "\xEA") {
        $val = ord(substr($col, 1,1));
        $bold = "bold_" if $val & 8;
        $fg = $Colors[$val & 7];
        $bg = "_" . $Colors[($val>>4) & 7];
        $bg = "" if $bg eq "Black";

        return "$bold$fg$bg";
    } elsif (substr($col,0,1) eq "\xEB") {
      return "End";
    }
    return "";
}

# replace ANSI colors with strings like {bold_White_Black} and print it out.
sub str_deansify {
  while(m/\G([^\xEA\xEB]*)(\xEA.|\xEB)?([^\xEA\xEB]*?)/g) {
    if(defined $2) {
      print $1 . "{" . &color_to_str($2) . "}" . $3;
    } else {
      print $1;
    }
  }
  print "\n";
}

# Example of use:
print "Loaded sys/color.pl\t(The ${Bold_Red}C${Green}O${Bold_Blue}L${Cyan}O${Magenta}U${Bold_Yellow}R${White} module)\n";
# print "Let's try some", setColor($White, $Green), " background${White} color. \n";

1;
