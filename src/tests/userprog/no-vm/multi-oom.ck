# -*- perl -*-
use strict;
use warnings;
use tests::tests;

our ($test);
my (@output) = read_text_file ("$test.output");
common_checks ("run", @output);

@output = get_core_output ("run", @output);
my ($n) = 0;
while (my ($m) = $output[0] =~ /^\(multi-oom\) begin (\d+)$/) {
    fail "Child process $m started out of order.\n" if $m != $n;
    $n = $m + 1;
    shift @output;
}
fail "Only $n child process(es) started.\n" if $n < 15;

# There could be a death notice for a process that didn't get
# fully loaded, and/or notices from the loader.
while (@output > 0
       && ($output[0] =~ /^multi-oom: exit\(-1\)$/
	   || $output[0] =~ /^load: /)) {
    shift @output;
}

while (--$n >= 0) {
    fail "Output ended unexpectedly before process $n finished.\n"
      if @output < 2;

    local ($_);
    chomp ($_ = shift @output);
    fail "Found '$_' expecting 'end' message.\n" if !/^\(multi-oom\) end/;
    fail "Child process $n ended out of order.\n"
      if !/^\(multi-oom\) end $n$/;

    chomp ($_ = shift @output);
    fail "Kernel didn't print proper exit message for process $n.\n"
      if !/^multi-oom: exit\($n\)$/;
}
fail "Spurious output at end: '$output[0]'.\n" if @output;

pass;
