Functions = ["load", "open", "close", "reopen", "quit", "speedwalk",
"bell", "echo", "status", "exec", "window", "kill", "print", "alias",
"action", "send", "help", "eval", "run", "setinput", "clear", "prompt"]

scolon_re_regexp = re.compile(r";")
newline_re_regexp = re.compile(r"\n")

def run(args):
  scolon_re_regexp.sub(r"\:", args)
  newline_re_regexp.sub(r" ", args)
  args = string.join((args, "\n"), "")
  os.write(interpreterPipe, args)

for func in Functions:
  exec("def dirt_" + func + "(*args):\n"
       "  str = \"\"\n"
       "  for i in range(len(args)):\n"
       "    str = string.join((str, args[i]), \"\")\n"
       "  run(\"#\" \"" + func + "\" \" \" + str)\n", globals(), globals())
