# Sample module for snarfing a prompt and displaying it in a window

# Usage: gauge(count, min,max, cur)
$emptyChar = '°';
$filledChar = '²';
sub gauge ($$$$) {
    my ($count, $min, $max, $cur, $n) = @_;
    $n = (($cur - $min) / ($max-$min)) * $count;
    $n = $0 if $n < 0;
    $n = $count if $n > $count;
    return ($filledChar x $n) . ( $emptyChar x ($count-$n));
}   

# This assumes a prompt that looks like this:
# minHp/maxHp minMana/maxMana minMove/maxMove
sub check_prompt {
    if (m#^(\d+)/(\d+)\s+(\d+)/(\d+)\s+(\d+)/(\d+)#) {
        &dirt_clear("prompt");
        &dirt_print("prompt Hp: ", &gauge(10, 1, $2, $1), "($1/$2)\n");
        &dirt_print("prompt Ma: ", &gauge(10, 1, $4, $3), "($3/$4)\n");
        &dirt_print("prompt Mv: ", &gauge(10, 1, $6, $5), "($5/$6)");
    } else {
        # Let a few prompts splip by if creating a character or such
        print ("Prompt failed to match regexp: $_") if ++$failedPrompts > 5;
    }
}


prompt_add(\&check_prompt);
&dirt_window("-w25 -x72 -y4 -h3 -B -c36 prompt");
