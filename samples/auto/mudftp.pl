# mudFTP support - push mode only, no caching support yet, autologin supported
#
# Oliver Jowett <icecube@ihug.co.nz>, 10/02/99

package mudFTP;

use Socket;
use Fcntl;
use POSIX qw(:sys_wait_h :errno_h);

# some configurable bits
$Temp = "/tmp/mudftp.";
$IdleTimeout = 600;

# internal vars
$Editor = "";
%Config = ();
    
$Filename = "";
$LinesRead = 0;
$LinesTotal = 0;
$EditorFile = "";
$EditString = "";
$State = 'Disabled';
$Buffer = "";
$PID = -1;

$AutoHost = 0;
$AutoPort = 0;
$AutoID = "";

$HasSocket = 0;

$IdleCount = 0;

$thisAccount = "";
$thisPW = "";

$active = 0;

sub Message {
    &main::run ($main::commandCharacter . "print mudFTP mudFTP: " . $_[0]);
}

sub SetState {
    $State = $_[0];
    Message("$State");
}

sub Shutdown {
    if ($PID != -1) {
        kill 15, $PID;
        $PID = -1;
        wait;
    }

    if ($EditorFile ne "") {
        unlink($EditorFile);
        $EditorFile = "";
    }
    
    if ($HasSocket) {
        close(MUDFTP);
        $HasSocket = 0;
    }
    
    $Buffer = "";
}

sub Error {
    Message("ERROR: " . $_[0] . "\n");
    SetState('Idle');
    Shutdown();
}

sub SendCmd {
    my $str = $_[0] . "\012";
    my $nwrite = syswrite(MUDFTP, $str, length($str));
    if (!defined($nwrite) || $nwrite != length($str)) {
        Error("Command write error: $!");
    }
}
    
sub ReadConfig {
    # read ~/.mudftp

    SetState('Idle');
    
    my $configfile = $main::ENV{HOME} . "/.mudftp";
    
    if (!open (IN, $configfile)) {
        Message("can't read $configfile: $!");
        return;
    }
    
    while (<IN>) {
        if (/^\s*mud\s+(\S+)\s+(\S+)\s+(\S+)\s+(\S+)\s+(\S+)/) {
            my @data = ($2, $3, $4, $5);
            $Config{$1} = \@data;
        } elsif (/^\s*editor\s+(.*)$/) {
            $Editor = $1;
        }
    }

    close(IN);

    if ($Editor eq "") {
        print "No editor defined, mudftp disabled.\n";
        SetState('Disabled');
    }
}

sub Connect {
    # Connect to the current mud, turn on push mode

    if ($HasSocket) {
        Shutdown();
    }
    
    my ($host, $port);
    
    if (exists($Config{$main::mud})) {
        ($host, $port, $thisAccount, $thisPW) = @{$Config{$main::mud}};
    } elsif ($AutoPort) {
        $host = $AutoHost;
        $port = $AutoPort;
        $thisAccount = "*";
        $thisPW = $AutoID;
    } else {
        Error("No config info for $main::mud");
        return;
    }

    my $proto = getprotobyname('tcp');
    my $ip = gethostbyname($host);

    if (!$ip) {
        Error("Unknown host: $host");
        return 0;
    }

    my $paddr = sockaddr_in($port, $ip);

    if (!socket(MUDFTP, PF_INET, SOCK_STREAM, $proto)) {
        Error("Socket error: $!");
        return 0;
    }

    $HasSocket = 1;
    
    if (!fcntl(MUDFTP, F_SETFL, O_NONBLOCK)) {
        Error("Fcntl error: $!");
        return 0;
    }        
    
    if (!connect(MUDFTP, $paddr)) {
        if ($! != &EINPROGRESS) {
            Error("connect error: $!");
            return 0;
        }
    }

    SetState('Connecting');
}

sub CheckCompleteRx {
    # See if we're done receiving a file

    if ($LinesRead == $LinesTotal) {
        # Read complete. Create temp file, spawn editor

        # bleh, perl has no tmpnam() equivalent?

        $EditorFile = $Temp . $main::now . $$;

        open(EDITFILE, ">$EditorFile") || do {
            Error("Can't open $EditorFile: $!");
            return;
        };

        print EDITFILE "$EditString";
        close(EDITFILE) || do {
            Error("Error writing to $EditorFile: $!");
            return;
        };

        # spawn editor

        my $cmdline = $Editor;
        $cmdline =~ s/%/$EditorFile/g;

        unless ($PID = fork()) {
            # Child

            exec($cmdline);
            exit(255); # erk!
        }

        SetState('Waiting for editor');
    }
}

sub Consume {
    $Buffer =~ s/^(.*)\012//;
    $active = 1;
}

sub Read {
    while (1) {
        my $nread = sysread(MUDFTP, $Buffer, 4096, length($Buffer));

        if (!defined($nread)) {
            if ($! != &EAGAIN) {
                Error("read error: $!");
            }
            return;
        }

        if ($nread == 0) {
            if (length($Buffer) == 0) {
                Error("read error: EOF from server");
            }
            return;
        }
    }
}

sub BeginPut {
    # Read and count lines

    if (!open (IN, "<$EditorFile")) {
        Error("Can't reopen edited file: $!");
        return;
    }

    $EditString = "";
    my $lines = 0;
    while (<IN>) {
        $lines++; $EditString .= $_;
    }
    close(IN);

    # Is the last line unterminated?
    if ($EditString !~ /\n$/) {
        $EditString .= "\n";
    }

    unlink($EditorFile);
    $EditorFile = "";
    
    SendCmd("PUT $Filename $lines");
    SetState('Sending file');
    $active = 1;
}

sub PutFile {
    while (1) {
        my $nwrite = syswrite(MUDFTP, $EditString, length($EditString));
        if (!defined($nwrite)) {
            if ($! != &EAGAIN) {
                Error("Write error: $!");
            }
            return;
        }

        if ($nwrite == 0) {
            return 0;
        }
        
        if ($nwrite > 0) {
            $EditString = substr($EditString, $nwrite);
            $active = 1;
        }

        if ($EditString eq "") {
            SetState('Waiting for server');
        }
    }
}

# Core FSM

# arg0 = pending line (maybe undef)
# arg1 = clear to write?

sub UpdateFSM {
    my ($line, $canwrite) = @_;
    
    for ($State) {
        /^Connecting/ && do {
            if ($canwrite) {
                SendCmd("$thisAccount $thisPW");
                SetState('Authenticating');
            }
            last;
        };

        /^Authenticating/ && do {
            if (defined($line)) {
                Consume();  # eat the line
                if ($line =~ /^OK/) {
                    SendCmd("PUSH");
                    SetState('Setting PUSH mode');
                } else {
                    Error("Authentication failed: " . $line);
                }
            }
            last;
        };

        /^Setting PUSH mode/ && do {
            if (defined($line)) {
                Consume();  # eat the line
                if ($line =~ /^OK/) {
                    SetState('Waiting for requests');
                } else {
                    Error("Couldn't set push mode: " . $line);
                }

                $IdleCount = 0;
            }
            last;
        };

        /^Waiting for requests/ && do {
            if (defined($line)) {
                Consume();  # eat the line
                if ($line =~ /^SENDING (\S+) (\d+) (\S+)$/) {
                    $Filename = $1;
                    $LinesTotal = $2;
                    $Checksum = $3;
                    $LinesRead = 0;
                    $EditString = "";
                    SetState('Reading request');

                    CheckCompleteRx();
                } else {
                    Error("Unexpected request: " . $line);
                }
            } elsif ($IdleTimeout) {
                $IdleCount++;

                if ($IdleCount > $IdleTimeout) {
                    $IdleCount = 0;
                    SendCmd("NOOP");
                    SetState('Waiting for keepalive');
                }
            }

            last;
        };

        /^Waiting for keepalive/ && do {
            if (defined($line)) {
                if ($line =~ /^SENDING/) {
                    # Oops, they sent us something just as we did a noop
                    $IdleCount = -1;   # magic value
                    SetState('Waiting for requests');
                    $active = 1;
                    last;
                }

                if ($line =~ /^OK/) {
                    Consume();
                    $IdleCount = 0;
                    SetState('Waiting for requests');
                    last;
                }

                Error("Keepalive no-op failed: $line");
                last;
            }

            $IdleCount++;
            if ($IdleCount > 60) {
                Error("Server failed to respond to keepalive");
            }
            
            last;
        };
                    
            
        /^Reading request/ && do {
            if (defined($line)) {
                Consume();  # eat the line
                $EditString .= $line . "\n";
                $LinesRead++;
                CheckCompleteRx();
            }
            last;
        };

        /^Waiting for editor/ && do {
            my $died = waitpid($PID, WNOHANG);

            if ($died > 0) {
                # Editor exited
                $PID = -1;
                
                if ($? != 0) {
                    if (WIFEXITED($?)) {
                        if (WEXITSTATUS($?) == 255) {
                            $status = "(exec failed, editor not found?)";
                        } else {
                            $status = "with status " . WEXITSTATUS($?);
                        }                            
                    } else {
                        $status = "via signal " . WTERMSIG($?);
                    }
                    
                    Message("Editor exited $status");
                    print "mudFTP: Editor exited $status\n"; # make sure the user sees this
                    SendCmd("STOP");
                    SetState('Waiting for server');
                } else {
                    # Read and send file
                    BeginPut();
                }
            }

            last;                
        };

        /^Sending file/ && do {
            if ($canwrite) {
                PutFile();   # keep going..
            }
            last;
        };
        
        /^Waiting for server/ && do {
            if (defined($line)) {
                # we're waiting for an OK / FAILED from the server after PUT/STOP
                Consume();  # eat the line
                
                # Better check for keepalives here
                if ($IdleCount == -1) {
                    if ($line =~ /^OK/) {
                        $IdleCount = 0;
                        last; # do nothing for now
                    }

                    Error("Pending keepalive failed: $line");
                    last;
                }
                
                if ($line =~ /^OK/) {
                    $IdleCount = 0;
                    SetState('Waiting for requests');
                } else {
                    Error("Server rejected edit: " . $line);
                }
            }
            last;
        };

        (/^Idle/ || /^Disabled/) && last;

        Error("In unhandled state: $State");
        last;
    }
}

# Idle function, runs the FSM

sub Idle {
    my $canwrite = 0;
    
    # See if we need to do a select
    if ($HasSocket) {
        my ($rin, $rout, $wout, $eout) = ("", "", "", "");
        vec($rin, fileno(MUDFTP), 1) = 1;

        if (select($rout=$rin, $wout=$rin, $eout=$rin, 0) > 0) {
            # ok, we maybe have data etc

            if (vec($rout, fileno(MUDFTP), 1)) {
                Read();
            }
            
            if ($HasSocket && vec($wout, fileno(MUDFTP), 1)) {
                # Write
                $canwrite = 1;
            }
                      
            if ($HasSocket && vec($eout, fileno(MUDFTP), 1)) {
                # Exception
                Error("Exception status on mudftp socket");                
            }
        }

    }

    # Now call the fsm

    do {
        # see if we have a line to handle
        my $nextline = undef;
        ($Buffer =~ /^(.*)\012/) && ($nextline = $1);
        $active = 0;
        
        UpdateFSM($nextline, $canwrite);
    } while ($active);
}

sub Detect {
    if (/^mudFTP supported on ([^:]+):(\d+) \(token ([^)]+)\)$/) {
        # Autologin support

        $AutoHost = $1;
        $AutoPort = $2;
        $AutoID = $3;

        Connect();
    }
        
    if (/^\S+\xE6$/) {
        # mudftp pull mode
        
        Connect();
    }

}

&main::run($main::commandCharacter . "window -w40 -h6 -x-0 -y3 -t5 mudFTP");

&ReadConfig();
#&main::hook_add("output", "mudFTP::Detect", \&Detect);
&main::run($main::commandCharacter . "hook -T OUTPUT -F -fL perl mudFTP_detect = mudFTP::Detect");
&main::callout_add(\&Idle, 1);
#&main::hook_add("done", "mudFTP::Shutdown", \&Shutdown);
&main::run($main::commandCharacter . "hook -T DONE -F -fL perl mudFTP_shutdown = mudFTP::Shutdown");

package main;

sub dirtcmd_mudftp {
    &mudFTP::Connect();
    return;
}

print "Loaded mudftp.pl\t(Routines to get/put files to/from certain muds)\n";

1;
