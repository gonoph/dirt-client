# Useful aliases

#You are carrying 0 coins in loose change.
#You have 0 coins in bags.
#You have 6056 coins in the bank.

# This is the command 'depositall' which will deposit all your coins if you're at an ATM.
sub cmd_depositall {
  my($coins) = 0;
  &output_add(sub {
    $coins = 
    if (/^You are carrying ([0-9]+) coins in loose change\.$/) { $coins += $1; }
    if (/^You have ([0-9]+) coins in bags\.$/) { $coins += $1; }
    if (/^You have ([0-9]+) coins in the bank\.$/) {
      if($coins > 15) {
        &mcl_send("deposit " . $coins-15);
      }
    }
  });
  &mcl_send("coins");
}
