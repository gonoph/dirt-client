# User variables module

# Let's go to another namespace to make things easier
%Vars =  ();

# Set a variable
sub dirtcmd_set {
        my ($var, $value) = split(' ', $_, 2);
        if (defined($value)) {
                $Vars{$var} = $value;
        } else {
                print "Variables set:\n";
                foreach (keys %Vars) {
			print "$_ = $Vars{$_}\n";
                }
        }

}

# Do variable substitution.
sub expand_vars {
	# Do variables first: $varname or ${varname}
	# Don't error if the variable doesn't exist, just ignore it
	s/(?<!\\)\$([_a-zA-Z]+)
	 /(defined($Vars{$1}) ? $Vars{$1} : "\$$1")
	 /gex;
	s/(?<!\\)\${([_a-zA-Z]+)}
	 /(defined($Vars{$1}) ? $Vars{$1} : "\$$1")
	 /gex;

	# Eval, usually for math stuff, but really any Perl: @{3 + 1} or even @{print "hello"}
	s/\@{(.*?)}
	 /eval $1
	 /gex;
}

#send_add(\&expand_vars);
&run($commandCharacter . "hook -T SEND -Fp 1000 -L perl variable_expansion = expand_vars");

# Add some color
{
    my @AnsiColors = qw /black red green yellow blue magenta cyan white/;
    my $i;
    for ($i = 0; $i < @AnsiColors; $i++) {
        $Vars{$AnsiColors[$i]} = "\e[" . ( 30 + $i) . "m";
        $Vars{"bold_" . $AnsiColors[$i]} = "\e[1;" . ( 30 + $i) . "m";
        $Vars{"bg_" . $AnsiColors[$i]} = "\e[" . ( 40 + $i) . "m";
    }
    $Vars{off} = "\e0m";
}

print "Loaded auto/vars.pl\t(Perform inline variable and subroutine substitution)\n";

1;
