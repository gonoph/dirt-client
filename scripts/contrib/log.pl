# Automatic logging of all incoming traffic
# Saves to a gzipped file in ~/.dirt/logs/sessionName/CurrentDate

use POSIX qw(strftime);
use Carp;

sub log_line {
    my ($line) = shift;
    $line =~ s/${ColorCode}.//gs;

    print GZLOG $line, "\n";
}

sub log_output {
    log_line($_);
}

sub short_timestamp {
    return strftime ("%H:%M:%S" ,localtime());
}

sub log_input {
    log_line ( sprintf("\n[%s] %s\n", short_timestamp(), $_) );
}

sub stop_log {
    output_remove("log_output");
    userinput_remove("log_input");
    loselink_remove("stop_log");
    close (GZLOG);
}

sub enable_log {
    my $logdir = "~/.dirt/logs/$mud";
    system("mkdir -p $logdir");
    my $filename = strftime("%Y-%b-%d.gz", localtime);
    my $logfile = "$logdir/$filename";
   
    if (open (GZLOG, "|exec /usr/bin/gzip -c >> $logfile")) {
        output_add("log_output");
        userinput_add("log_input");
        loselink_add("stop_log");
    } else {
        print "Error opening log file $logdir/$filename: $!\n";
    }
}

connect_add("enable_log");
