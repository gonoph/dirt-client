# Basic stuff for 3 kingdoms
#
# This is specific to the Three Kingdoms mud at 3k.org:5000
# If you want to use this, place this in your ~/.dirt/ directory and 
# create a MUD definition in the config file like this:
#     MUD ThreeKingdoms {
#         Host 3k.org 5000
#     }

package ThreeKingdoms;  # Creates my namespace, ThreeKingdoms
# Note that to use MCL stuff you must prefix it by main::

# GLOBALs used by this module
use vars qw(@room_exits);
@room_exits = ();                     # list of the exits to the current room.

# This replaces the standard prompt hook.
sub prompt_grab {
  while($_ =~ m/^> (.*)$/) {
    $_ = $1;
    &main::hook_run("prompt_hooks");
  }
}
&main::output_add(\&prompt_grab);

############################# ROOMS ###########################################
my(@dirs) = ("n", "e", "s", "w", "ne", "nw", "se", "sw", "u", "d", "vortex",
  "enter", "cave", "cavern", "out", "portal", "gate", "in", "mist", "left",
  "right", "out1", "out2", "house", "tree", "stairs", "ladder", "port", "stern",
  "forward", "hole", "alley", "leave", "tear", "guild", "chaos", "pub",
  "fantasy", "science", "newbie", "login", "shop", "bank", "smithy", "gypsy",
  "arundin", "office", "jump", "magic", "sci", "sinkhole", "door", "path",
  "flames", "omp", "gap", "arena", "equip", "exit", "forest", "tower", "archway",
  "arch"); 
my($roomnamecolor)     = &main::str_to_color("bold_White_Blue");
my($roomexitscolor)    = &main::str_to_color("Cyan_Blue");
my($dirsstring)        = join("|", @dirs);
my($bold_Yellow_Black) = &main::str_to_color("bold_Yellow_Black");
my($White_Black)       = &main::str_to_color("White_Black");
my($colorize_brief_rooms) = 1;
sub grab_room {
  if(/^${bold_Yellow_Black}    There (?:is|are) [a-z]+ obvious exits?: ((?:[a-z]+|, )+)${White_Black}$/) {
    @room_exits = split("\, ", $1);
    &main::hook_run("room_hooks");
  } elsif(/^(.* )(\((($dirsstring|,)+)\)).?$/) {
    if($colorize_brief_rooms) {
      $_ = "$main::CSave$roomnamecolor$1 $roomexitscolor$2$main::CRestore";
    }
    @room_exits = split(",", $3);
    &main::hook_run("room_hooks");
  }
}
&main::output_add(\&grab_room);
# Create a hook for rooms.  If you want your function to be called when you see a 
# room description, add this: &ThreeKingdoms::room_add(\&your_func_here);
&main::create_standard_hooks(1, "room");  
&main::save("ThreeKingdoms::colorize_brief_roosm", \$colorize_brief_rooms);

############################# PLAYERS ###########################################
# This highlights players in the room
my(@alignments) = ("good", "very good", "peeronic", "mean", "evil", "satanic", 
  "heavenly", "neutral", "moral", "saintly", "tensoric", "malicious", "immoral", 
  "pure", "demonic", "nice", "corrupt", "righteous", "sinister");
my($playercolor)       = &main::str_to_color("bold_White_Magenta");
my($alignmentcolor)    = &main::str_to_color("Cyan_Magenta");
my($alignmentstring)   = join("|", @alignments);
my($colorize_players)  = 1;
sub grab_players {
  if(/^(.*)(\((?:$alignmentstring)\)\.?)$/) {
    if($colorize_players) {
      $_ = "$main::CSave$playercolor$1$alignmentcolor$2$main::CRestore";
    }
    &main::hook_run("player_hooks");
  }
}
&main::output_add(\&grab_players);
# Create a hook for players.  If you want your function to be called when you see
# another player, add this: &ThreeKingdoms::player_add(\&your_func_here);
&main::create_standard_hooks(1, "player");
&main::save("ThreeKingdoms::colorize_players", \$colorize_players);








1;
