
# Cancel and restart compression when restarting
sub cancel_compression {
	dirt_send_unbuffered ("compress");
	dirt_send_unbuffered ("compress");
}

done_add(\&cancel_compression);
