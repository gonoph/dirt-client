
package Help;

my($DOCPATH) = $main::DIRT_HOME . "/lib/dirt/doc";
my($height) = 25;
my($viewpos) = 0;
my(@helpcontents);

# bind pgup (need Window-specific KEYBOARD hooks)
# bind pgdn
# bind Esc (kill window)
# When text is longer than the window, put prompt at bottom and pgup/pgdn hints.
$windowColor = &main::str_to_color('white_blue');
&main::run("/window -x 0 -y 10 -w -1 -h $height -H -t 0 -c 31 help");

# open the help window when alt-h is pressed.
&main::run("/hook -g 'Dirt keys' -k alt_h __DIRT_HELP_open = /run -Lperl Help::openhelpwindow");
sub openhelpwindow {
    &main::run("/window -s help");
    &main::run("/disable __DIRT_HELP_open");
    return 1;
}

# Close the help window when either esc or alt-h are pressed.
&main::run("/hook -g 'Dirt keys' -k alt_h -W help __DIRT_HELP_alt_h_close = /run -Lperl Help::closehelpwindow");
sub closehelpwindow {
    &main::run("/window -i help");
    &main::run("/enable __DIRT_HELP_open");
    return 1;
}
&main::run("/hook -g 'Dirt keys' -k escape -W help __DIRT_HELP_esc_close = /run -Lperl Help::closehelpwindow");

# Redraw window
sub redraw {
    if($height-2 > $#helpcontents) { $viewpos = 0; }
    elsif($viewpos + $height - 2 >= $#helpcontents) { # don't move...
    } else { $viewpos += $height - 4; } # 2 for border, 2 overlap

    for($i=$viewpos;$i<(($viewpos+$height-2>$#helpcontents)?$#helpcontents:$viewpos+$height-2);$i++) {
        &main::run("/echo -W help '" . &main::backslashify($helpcontents[$i], "'") . "'");
    }
    if($viewpos + $height - 2 >= $#helpcontents) { # no more help data, don't show [pgdn]
        &main::run("/status -W help 'Keys available:        [PgUp] [Up]        [Esc] [Alt-H]'");
    } else {
        &main::run("/status -W help 'Keys available: [PgDn] [PgUp] [Up] [Down] [Esc] [Alt-H]'");
    }
}

# Scrolling with page_down key
&main::run("/hook -g 'Dirt keys' -k page_down -W help __DIRT_HELP_pgdn = /run -Lperl Help::pgdn");
sub pgdn {
    &main::run("/clear help");
    if($height-2 > $#helpcontents) { $viewpos = 0; }
    elsif($viewpos + $height - 2 >= $#helpcontents) { # don't move...
    } else { $viewpos += $height - 4; } # 2 for border, 2 overlap
    for($i=$viewpos;$i<(($viewpos+$height-2>$#helpcontents)?$#helpcontents:$viewpos+$height-2);$i++) {
        &main::run("/echo -W help '" . &main::backslashify($helpcontents[$i], "'") . "'");
    }
    if($viewpos + $height - 2 >= $#helpcontents) { # no more help data, don't show [pgdn]
        &main::run("/status -W help 'Keys available:        [PgUp] [Up]        [Esc] [Alt-H]'");
    } else {
        &main::run("/status -W help 'Keys available: [PgDn] [PgUp] [Up] [Down] [Esc] [Alt-H]'");
    }
    return 1;
}

# Scrolling with page_up key
&main::run("/hook -g 'Dirt keys' -k page_up -W help help_pgup = /run -Lperl Help::pgup");
sub pgup {
    &main::run("/clear help");
    $viewpos -= $height - 4; # 2 for border, 2 overlap
    if($viewpos < 0) { $viewpos = 0; }
    for($i=$viewpos;$i<(($viewpos+$height-2>$#helpcontents)?$#helpcontents:$viewpos+$height-2);$i++) {
        &main::run("/echo -W help '" . &main::backslashify($helpcontents[$i], "'") . "'");
    }
    if($viewpos == 0) { # no more help data, don't show [pgdn]
        &main::run("/status -W help 'Keys available: [PgDn]             [Down] [Esc] [Alt-H]'");
    } else {
        &main::run("/status -W help 'Keys available: [PgDn] [PgUp] [Up] [Down] [Esc] [Alt-H]'");
    }
    return 1;
}

sub arrow_up {
    &main::run("/clear help");
    $viewpos -= 1;
}

&main::run("/hook -g 'Dirt commands' -T COMMAND -C help __DIRT_HELP_command = /run -Lperl Help::command_help");
sub command_help {
    &main::run("/clear help");
    if(/^.help (\w+)/) {
        my($fname) = "";
        if(-f "$DOCPATH/$1") { $fname = "$DOCPATH/$1"; }
        if(-f "$DOCPATH/commands/$1") { $fname = "$DOCPATH/commands/$1"; }
        if(!open(FILE, "<$fname")) { 
            &main::report_err($main::commandCharacter . "help: '$1' is not a valid help topic!"); 
            return(1); 
        }
        chomp(@helpcontents = <FILE>);
    }
    else {
        my(@files);
        @helpcontents = ();
        push @helpcontents, "Help topics available: ";
#        &main::run("/echo -W help Help topics available: ");
        my($longest) = 0;
        foreach $file (<$DOCPATH/*>) {
            if(-f $file) {
                $file =~ s/.*?\/([^\/]*)$/$1/;
                if(length($file) > $longest) { $longest = length $file; }
                push @files, $file;
            }
        }
        $longest++;
        my($line) = "\t";
        foreach $file (@files) {
            if((length($line) + $longest) > 80) {
                push @helpcontents, $line;
#                &main::run("/echo -W help $line");
                $line = "\t";
            }
            $line .= sprintf("%${longest}s", $file);
        }
        push @helpcontents, $line;
#        &main::run("/echo -W help '" . &backslashify($line) . "'");
        push @helpcontents, "Commands available: ";
#        &main::run("/echo -W help 'Commands available:'");
        @files = ();
        $longest = 0;
        foreach $file (<$DOCPATH/commands/*>) {
            if(-f $file) {
                $file =~ s/.*?\/([^\/]*)$/$1/;
                if(length($file) > $longest) { $longest = length $file; }
                push @files, $file;
            }
        }
        $longest++;
        $line = "\t";
        foreach $file (@files) {
            if((length($line) + $longest) > 80) {
                push @helpcontents, $line;
#                &main::run("/echo -W help '" . &backslashify($line) . "'");
                $line = "\t";
            }
            $line .= sprintf("%${longest}s", $file);
        }
        push @helpcontents, $line;
#        &main::run("/echo -W help '" . &backslashify($line) . "'");
    }
    $viewpos = 0;
    for($i=0;$i<(($height-2>$#helpcontents)?$#helpcontents:$height-2);$i++) {
        &main::run("/echo -W help '" . &main::backslashify($helpcontents[$i], "'") . "'");
    }
    if($#helpcontents > $height-2) {
        &main::run("/status -W help 'Keys available: [PgDn]             [Down] [Esc] [Alt-H]'");
    } else {
        &main::run("/status -W help 'Keys available:                           [Esc] [Alt-H]'");
    }
    &main::run("/window -s help");
    &main::run("/disable __DIRT_HELP_open");
}

