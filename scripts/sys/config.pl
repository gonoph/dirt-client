# Configuration manager

# Change if necessary
$CONFIG_FILE = "$ENV{HOME}/.dirt/Saved.pl";

# Initial loading of configuration
# Must be done after all auto/* modules have had a chance to run...
&run($commandCharacter . "hook -T INIT -F -p 2147483647 -fL perl load_configuration = load_configuration");
sub load_configuration {
    $Loading = 1;
    delete $INC {$CONFIG_FILE}; # tell perl we haven't already require'd it.
    if(!-f $CONFIG_FILE) { &save_configuration; }
    eval { require $CONFIG_FILE; };
    if($@) {
      print "Syntax error in config file ($CONFIG_FILE).  FIX IT!\n";
      return;
    } else {
      require $CONFIG_FILE;
    }
    $Loading = 0;
    &run($commandCharacter . "hook -d load_configuration");
}

&run($commandCharacter . "hook -T DONE -F -p -1 -fL perl save_configuration = save_configuration");
sub save_configuration {
    return if $ReadOnly;
    
    open(CONFIG, ">" . $CONFIG_FILE) or die "Could not open $CONFIG_FILE for writing: $!";
    print CONFIG "# Dirt module config file generated on ", scalar(localtime($now)), " by $VERSION\n";
    
    print "@@ Dumping \%Config to file\n";
    foreach my $var (keys %Config) {
      if(ref($Config{$var}) eq 'HASH') {
        print CONFIG Data::Dumper->Dump([$Config{$var}], ["*$var"]);
      } else {
        print CONFIG Data::Dumper->Dump([${$Config{$var}}], [$var]);
      }
    }
    print CONFIG "1;\n";
    close (CONFIG);
}

# Bob's additions
# A few utilities to help load/save simple options to config files.
%Config = (); # just to keep track of them

require Data::Dumper;
$Data::Dumper::Indent = 1;

# save("name", \$name)
# Saves variable "name" to the config file.
sub save {
  my($name)  = shift;
  my($value) = shift; # should be a ref.
  if(ref($value)) {
    $Config{$name} = $value;
  } else { # error...
    print "save($name, $value) second parameter must be a ref to the object to be saved.\n";
  }
}

print "Loaded config.pl\t(Routines to load/save perl config)\n";

1;
