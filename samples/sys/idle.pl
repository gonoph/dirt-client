# Callout management

@Callouts = ();
@CalloutTimes = ();
@NextCallout = ();

sub idle {
    my ($i);
    for ($i = 0; $i <= $#Callouts; $i++) {
        if ($NextCallout[$i] <= $now) {
            $NextCallout[$i] = $now + $CalloutTimes[$i];
            &{$Callouts[$i]};
        }
    }
}

sub callout_add ($$) {
    my ($function, $time) = @_;
    push @Callouts, $function;
    push @CalloutTimes, $time;
    push @NextCallout, $now + $time;
}

sub callout_once ($$) {
    my ($function, $time, $sub) = @_;
    $sub = sub { callout_remove($sub); &$function(); };
    callout_add ($sub, $time);
}

sub callout_remove ($) {
    my ($what, $x) = @_;
    for ($x = 0; $x <= $#Callouts; $x++) {
        # Is this the right way to compare references?
        if ($Callouts[$x] eq $what) {
            splice(@Callouts, $x, 1);
            splice(@CalloutTimes, $x, 1);
            splice(@NextCallout, $x, 1);
            return;
        }
    }

    print "callout_remove(): Removal of $what FAILED!\n";
}

sub cmd_show_callouts {
    my ($i);
    foreach $i (0 .. $#Callouts) {
        printf "%-20s %6d %6d\n", $Callouts[$i], $CalloutTimes[$i], $NextCallout[$i];
    }
}

print "Loaded sys/idle.pl\t(Routines to call things on a regular basis)\n";

1;
