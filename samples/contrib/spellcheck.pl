# Spellchecker which interfaces to ispell

# The userinput hook will fix the words as you type
# but, only when they start with certain patterns
@fixablePrefixes = qw/: ' say tell immtalk muse tech reply shout yell whisper emote remote btell pray beseech/;


use IPC::Open2;
$spellPID = open2(*spellReader, *spellWriter, "ispell -A -S -C");
<spellReader>; # Get rid of the (C) line

# Important to kill this when exiting!
done_add(sub {kill 15, $spellPID; close (spellReader); close (spellWriter);wait;});

# Don't check anything containing this
$specialSymbols = '@$_\*';

sub checkSpelling ($) {
    my ($word, $res) = @_;

    return $word if $word =~ /\d/ or $word =~ /[$specialSymbols]/ or $word =~/^\W+$/;

    print spellWriter "$word\n";
    $res = <spellReader>; <spellReader>; # Emtpy line
    chomp $res;

    $word = $2 if $res =~ /^& \w+ (\d+) \d+: ([^,]*)/;

    return $word;
}

# Check all words in this word for spelling. Report to stdout
sub check_words {
    my ($x,$rep,$out,$count,$p) = ("","","",0,"");

    # ugh, nesting hell

    $_[0] .= "\xFE";  # mark real end
    $_[0] =~ s/^(\s*\w)/uc $1/ge;      # First word
    foreach $p (split(/`([^`]*)`?/, $_[0])) {
        # don't try to correct anything in ``s, and remove them
        
        if (!($count % 2)) {
            # don't correct anything in "", but leave the punctuation

            my ($count2,$q) = (0, "");
            foreach $q (split(/("[^"]*"?)/, $p)) {
                if (!($count2 % 2)) {

                    $q =~ s/(\.\s*\w)/uc $1/ge;     # Capitalize sentences.
                    $q =~ s/(([\(\)]:)|[^.!?])\s*\xFE/$1./;      # Add . at the end
                    $q =~ s/\bi\b/I/g;              # capitalize i

                    $q =~ s/([\w'${specialSymbols}]+)/$x = &checkSpelling($1); $rep .= "$1 => $x, " unless $x eq $1; $x/ge;

                }
                
                $out .= $q;
                $count2++;
            }
        } else {
            $out .= $p;
        }
        $count++;
    }

    if (length($rep)) {
        substr($rep,-2,2) = "";
        print "$rep\n";
    }

    $out =~ s/\xFE//g;  # just in case
    $_[0] = $out;
}

# For testing things yourself
sub dirtcmd_spellcheck {
    &check_words($_);
}

# Command '/addword' adds a word to the dictionary
sub dirtcmd_addword {
    if (/^(\w+)/) {
        print spellWriter "*$1\n#\n";
        print "Added word $1 to the dictionary.\n";
    } else {
        print "Add what word to the dictionary?\n";
    }
}

# Callback that checks the user's input
sub fix_input {
    my ($x,$tmp);
    foreach $x (@fixablePrefixes) {
        if (/^$x(.*)/) {
            $tmp = $1;
            check_words($tmp);
            $_ = $x . $tmp;
            last;
        }
        
    }
}

userinput_add(\&fix_input);
