# just a simple little bot that casts spells on demand
# spell keywords and data to be sent with a name appended
%bot_spells = ("slink" => "slink",
               "fly" => "fly");

$bot_curr_hp = 0;
$bot_max_hp = 0;
$bot_curr_mana = 0;
$bot_max_mana = 0;
$bot_curr_move = 0;
$bot_max_move = 0;
$prompt_read = 0;
$regen = 0;

# pattern to match my prompt on ROD, you'll probably need to change this
# the [^\\d]s are too get rid of colors so those extra chars don't cause problems
# ex: <123/123hp 123/123mana 123/123mv>
$prompt_pattern = "<[^\\d]*(\\d+)/(\\d+)[^\\d]+(\\d+)/(\\d+)[^\\d]+(\\d+)/(\\d+)[^>]*>";

sub bot_prompt_hook
{
  my $data = $_;

  if($data =~ m|$prompt_pattern|) {
    $prompt_read = 1;
    $bot_curr_hp = $1;
    $bot_max_hp = $2;
    $bot_curr_mana = $3;
    $bot_max_mana = $4;
    $bot_curr_move = $5;
    $bot_max_move = $6;
  }
}

sub bot_hook
{
  my $data = $_;
  my $backup = $_;

  if($bot_curr_hp < 20 && $prompt_read) {
    print("\nHitpoints too low, quitting...\n");
    &bot_command("quit");
    $prompt_read = 0;
    return;
  }
  if($regen) {
    if($bot_curr_mana == $bot_max_mana) {
      &done_regen();
    }
    else {
      return;
    }
  }
  elsif($data eq "You don't have enough mana.") {
    &regen();
  }
  elsif($data =~ /([^ ]+) whispers[^\']+\'([^\']+)\'/) {
    &handle_whisper($1, $2);
  }
  elsif($data =~ /([^ ]+) tell[^\']+\'([^\']+)\'/) {
    &handle_tell($1, $2);
  }

  $_ = $backup;
}

sub regen
{
  &bot_command("report");
  &bot_command("sleep");
  $regen = 1;
}

sub done_regen
{
  &bot_command("wake");
  &bot_command("report");
  $regen = 0;
}

sub handle_whisper
{
  my ($name, $args) = @_;
  my $spell;

  if($args eq "status") {
    &status_reply($name, \&whisper_to);
  }
  else {
    foreach $spell (keys(%bot_spells)) {
      if($args =~ /^$spell ?$/) {
        &cast_spell($name, $spell);
      }
    }
  }
}

sub handle_tell
{
  my ($name, $args) = @_;

  &status_reply($name, \&tell_to) if($args eq "status");
} 

sub cast_spell
{
  my ($target, $spell) = @_;

  &dirt_send("c \'".$bot_spells{$spell}."\' $target");
}

sub status_reply
{
  my ($name, $func) = @_;

  &$func($name, "Current hp: $bot_curr_hp");
  &$func($name, "Current mana: $bot_curr_mana");
  &$func($name, "Current move: $bot_curr_move");
}

sub bot_command
{
  &dirt_send($_[0]);
}

sub whisper_to
{
  my($name, $args) = @_;

  &bot_command("whisper $name $args");
}

sub tell_to
{
  my($name, $args) = @_;
 
  &bot_command("tell $name $args");
}

&output_add(\&bot_hook);
&prompt_add(\&bot_prompt_hook);

1;
