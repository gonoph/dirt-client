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
	s/\$(\w+)
         /die "Undefined variable $1 used" unless defined $Vars{$1};
          $Vars{$1}
         /gex;
	s/\$\((\w+)\)
         /die "Undefined variable $1 used" unless defined $Vars{$1};
          $Vars{$1}
         /gex;

	s/\@{(.*?)}/eval $1/ge;
}

send_add(\&expand_vars);

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




