# Dirt module config file generated on Wed Mar  7 10:05:02 2001 by 0.53.00
$Completion::AutocompleteMin = 4;
$ThreeKingdoms::colorize_brief_roosm = 1;
%Gag::Gags = (
  '^Cast a shocking grasp at whom\\?' => 13484,
  '^Cast a burning hands at whom\\?' => 1619,
  'follows Zathras into the room\\.$' => 15455,
  '^Cast a acid arrow at whom\\?' => 4391,
  '^Cast a magic missile at whom\\?' => 3546
);
%Highlight::Highlights = (
  '<Breed>[a-zA-z ]*:' => 'green',
  '<Announce>' => 'bold_green',
  '^<Breed>' => 'bold_green',
  '\\[Auction\\]' => 'magenta',
  '[Zz]athras' => 'white_red'
);
$Completion::AutocompleteSmashCase = 1;
$Completion::AutocompleteSize = 200;
$ThreeKingdoms::colorize_players = 1;
%Trigger::Triggers = (
  '^There is darkness as a Orb of Light goes dark.' => [
    'cast light',
    1
  ],
  '^The Orb of Light goes dark\\.' => [
    'cast light',
    1
  ],
  '^  -> Your deeppocket begins to fade... <-' => [
    'cast deeppockets',
    1
  ],
  '^Vilgan arrives following you\\.' => [
    'abuse vilgan',
    1
  ],
  '^(Zathras|[A-Za-z]+ golem|Orc|Manes|Kobold|Hobgoblin|Goblin|Giant rat) dealt the killing blow to' => [
    'corpse;ga;paip',
    1
  ]
);
1;
