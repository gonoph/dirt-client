# Automatically reconnect when the MUD drops the link on you
# THIS IS NOT WORKING TOO WELL. USE AT OWN RISK.

# Commands that quell reconnection for 30 seconds
@QuellCommands = qw/quit reboot/;
$quell_reconnect_time = 0;

sub reconnect_action {
    return if $now < $quell_reconnect_time;

    # Dont try again for a while
    $quell_reconnect_time = $now + 30;
    print "Reconnecting in 5 seconds.\n";
    $ReconnectCancelled = 0;
    &callout_once(sub {&dirt_reopen(); }, 5);
}

sub quell_reconnect {
    print "Quelling reconnect.\n";
    $quell_reconnect_time = $now + 30;
    return 0;
}

&loselink_add(\&reconnect_action);

foreach (@QuellCommands) {
    &command_add(\&quell_reconnect,$_);
}

1;
