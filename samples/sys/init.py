import sys
import os
import time
import string
import re

top_script = os.environ["HOME"] + "/.dirt/"
default_var = ""

def load_script(script_name):
  try:
    execfile(top_script + script_name, globals(), globals())
  except IOError:
    print "Couldn't load script: " + script_name

def init():
  load_script("sys/hook.py")
  load_script("sys/functions.py")
  load_script("sys/idle.py")
  load_script("sys/config.py")

  for file_name in os.listdir(top_script + "auto"):
    if file_name[-2:] == "py":
      load_script("/auto/" + file_name)
  load_configuration()
  done_add( save_configuration )
