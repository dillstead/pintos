# -*- perl -*-
use strict;
use warnings;
use tests::tests;

our ($test);
my (@output) = read_text_file ("$test.output");

common_checks (@output);
@output = get_core_output (@output);

my ($begin);
for my $i (0...$#output) {
    $begin = $i, last if $output[$i] eq '(dir-lsdir) begin';
}
fail "\"(dir-lsdir) begin\" does not appear in output\n" if !defined $begin;

my ($end);
for my $i (0...$#output) {
    $end = $i, last if $output[$i] eq '(dir-lsdir) end';
}
fail "\"(dir-lsdir) end\" does not appear in output\n" if !defined $end;
fail "\"begin\" follows \"end\" in output\n" if $begin > $end;

my (%count);
for my $fn (@output[$begin + 1...$end - 1]) {
    $fn =~ s/\s+$//;
    fail "Unexpected file \"$fn\" in lsdir output\n"
      unless grep ($_ eq $fn, qw (. .. dir-lsdir));
    fail "File \"$fn\" listed twice in lsdir output\n"
      if $count{$fn};
    $count{$fn}++;
}
fail "No files in lsdir output\n" if scalar (keys (%count)) == 0;
fail "File \"dir-lsdir\" missing from lsdir output\n"
  if !$count{"dir-lsdir"};

pass;
