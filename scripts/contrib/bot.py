bot_spells = {"slink" : "slink",
              "fly" : "fly"}

bot_curr_hp = 0
bot_max_hp = 0
bot_curr_mana = 0
bot_max_mana = 0
bot_curr_move = 0
bot_max_move = 0
prompt_read = 0
regen_mode = 0

prompt_pattern = re.compile(r"<[^\d]*(\d+)/(\d+)[^\d]+(\d+)/(\d+)[^\d]+(\d+)/(\d+)[^>]*>")
whisper_pattern = re.compile(r"([^ ]+) whispers[^\']+\'([^\']+)\'")
tell_pattern = re.compile(r"([^ ]+) tell[^\']+\'([^\']+)\'")

def bot_prompt_hook():
  global bot_curr_hp, bot_max_hp, bot_curr_mana, bot_max_mana
  global bot_curr_move, bot_max_move, prompt_read
  pp_match = prompt_pattern.search(default_var)
  
  if pp_match != None:
     bot_curr_hp, bot_max_hp = pp_match.group(1, 2)
     bot_curr_mana, bot_max_mana = pp_match.group(3, 4)
     bot_curr_move, bot_max_move = pp_match.group(5, 6)
     prompt_read = 1

def bot_hook():
  global prompt_read
  args = default_var
  if bot_curr_hp < 20 and prompt_read:
    print "\nHitpoints are too low, quitting..."
    bot_command("quit")
    prompt_read = 0
    return
  if regen_mode:
    if bot_curr_mana == bot_max_mana:
      done_regen()
      return
  else:
    if args == "You don't have enough mana.":
      regen()
      return
  regexp_match = whisper_pattern.search(args)
  if regexp_match:
    handle_whisper(regexp_match.group(1), regexp_match.group(2))
  else:
    regexp_match = tell_pattern.search(args)
    if regexp_match:
      handle_tell(regexp_match.group(1), regexp_match.group(2))

def regen():
  global regen_mode
  bot_command("report")
  bot_command("sleep")
  regen_mode = 1

def done_regen():
  global regen_mode
  bot_command("wake")
  bot_command("report")
  regen_mode = 0

def handle_whisper(name, msg):
  if msg == "status":
    status_reply(whisper_to, name)
  else:
    for spell in bot_spells.keys():
      spell_regexp = re.compile("^" + spell + " ?$")
      if spell_regexp.search(msg):
        cast_spell(name, spell)

def handle_tell(name, msg):
  if msg == "status":
    status_reply(tell_to, name)

def status_reply(func, name):
  func(name, "Current hp: " + `bot_curr_hp`)
  func(name, "Current mana: " + `bot_curr_mana`)
  func(name, "Current move: " + `bot_curr_move`)

def bot_command(command):
  dirt_send(command)

def whisper_to(name, msg):
  dirt_send("whisper " + name + " " + msg)

def tell_to(name, msg):
  dirt_send("tell " + name + " " + msg)

def cast_spell(target, spell):
  if bot_spells.has_key(spell):
    dirt_send("c \'" + bot_spells[spell] + "\' " + target)

prompt_add(bot_prompt_hook)
output_add(bot_hook)
