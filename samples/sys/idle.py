Callouts = []

class Callout:
  def __init__(self, function, time, val = 0):
    self.once = val
    self.func = function
    self.next_call = now + time
    self.next_time = time

  def once_only(self):
    if self.once:
      return 1
    else:
      return 0
  
  def call(self):
    self.func()
    self.next_call = now + self.next_time

  def ready(self):
    if self.next_call <= now:
      return 1
    else:
      return 0

def idle():
  for i in range(len(Callouts)):
    if Callouts[i].ready():
      Callouts[i].call()
      if Callouts[i].once_only():
        del Callouts[i]

def callout_add(function, time):
  callout = Callout(function, time)
  Callouts.append(callout)

def callout_remove(function):
  for i in range(len(Callouts)):
    if Callouts[i].func is function:
      del Callouts[i]

def callout_once(function, time):
  callout = Callout(function, time, 1)
  Callouts.append(callout)

def cmd_show_callouts():
  for i in range(len(Callouts)):
    print "%-20s %6d %6d\n" % (Callouts[i].func, Callouts[i].next_time, Callouts[i].next_call) 
