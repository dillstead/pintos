# -*- perl -*-
use strict;
use warnings;
use tests::tests;

our ($test);
my (@output) = read_text_file ("$test.output");

common_checks (@output);
@output = get_core_output (@output);

must_contain_in_order (\@output,
		       '(dir-lsdir) open .',
		       '(dir-lsdir) isdir(.)',
		       '(dir-lsdir) close .');

sub must_contain_in_order {
    my ($output, @lines) = @_;
    my (@line_numbers) = map (find_line ($_, @$output), @lines);
    for my $i (0...$#lines - 1) {
	fail "\"$lines[$i]\" follows \"$lines[$i + 1]\" in output\n"
	  if $line_numbers[$i] > $line_numbers[$i + 1];
    }
}

sub find_line {
    my ($line, @output) = @_;
    for my $i (0...$#output) {
	return $i if $line eq $output[$i];
    }
    fail "\"$line\" does not appear in output\n";
}

my (%count);
for my $fn (map (/readdir: \"([^"]+)\"/, @output)) {
    fail "Unexpected file \"$fn\" in lsdir output\n"
      unless grep ($_ eq $fn, qw (dir-lsdir));
    fail "File \"$fn\" listed twice in lsdir output\n"
      if $count{$fn};
    $count{$fn}++;
}
fail "No files in lsdir output\n" if scalar (keys (%count)) == 0;
fail "File \"dir-lsdir\" missing from lsdir output\n"
  if !$count{"dir-lsdir"};

pass;
