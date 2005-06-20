use strict;
use warnings;
use tests::Algorithm::Diff;

sub fail;
sub pass;

die if @ARGV != 2;
our ($test, $src_dir) = @ARGV;
our ($src_stem) = "$src_dir/$test";

our ($messages) = "";
open (MESSAGES, '>', \$messages);
select (MESSAGES);

sub check_expected {
    my ($expected) = pop @_;
    my (@options) = @_;
    my (@output) = read_text_file ("$test.output");
    common_checks (@output);
    compare_output (@options, \@output, $expected);
}

sub common_checks {
    my (@output) = @_;

    fail "No output at all\n" if @output == 0;

    check_for_panic (@output);
    check_for_keyword ("FAIL", @output);
    check_for_triple_fault (@output);
    check_for_keyword ("TIMEOUT", @output);

    fail "Didn't start up properly: no \"Pintos booting\" startup message\n"
      if !grep (/Pintos booting with.*kB RAM\.\.\./, @output);
    fail "Didn't start up properly: no \"Boot complete\" startup message\n"
      if !grep (/Boot complete/, @output);
    fail "Didn't shut down properly: no \"Timer: # ticks\" shutdown message\n"
      if !grep (/Timer: \d+ ticks/, @output);
    fail "Didn't shut down properly: no \"Powering off\" shutdown message\n"
      if !grep (/Powering off/, @output);
}

sub check_for_panic {
    my (@output) = @_;

    my ($panic) = grep (/PANIC/, @output);
    return unless defined $panic;

    print "Kernel panic: ", substr ($panic, index ($panic, "PANIC")), "\n";

    my (@stack_line) = grep (/Call stack:/, @output);
    if (@stack_line != 0) {
	my (@addrs) = $stack_line[0] =~ /Call stack:((?: 0x[0-9a-f]+)+)/;
	print "Call stack: @addrs\n";
	print "Translation of call stack:\n";
	print `backtrace kernel.o @addrs`;
    }

    if ($panic =~ /sec_no \< d-\>capacity/) {
	print <<EOF;
\nThis assertion commonly fails when accessing a file via an inode that
has been closed and freed.  Freeing an inode clears all its sector
indexes to 0xcccccccc, which is not a valid sector number for disks
smaller than about 1.6 TB.
EOF
    }

    fail;
}

sub check_for_keyword {
    my ($keyword, @output) = @_;
    
    my ($kw_line) = grep (/$keyword/, @output);
    return unless defined $kw_line;

    # Most output lines are prefixed by (test-name).  Eliminate this
    # from our message for brevity.
    $kw_line =~ s/^\([^\)]+\)\s+//;
    print "$kw_line\n";

    fail;
}

sub check_for_triple_fault {
    my (@output) = @_;

    return unless grep (/Pintos booting/, @output) > 1;

    print <<EOF;
Pintos spontaneously rebooted during this test.
This is most often caused by unhandled page faults.
EOF

    fail;
}

# Get @output without header or trailer.
sub get_core_output {
    my ($p);
    do { $p = shift; } while (defined ($p) && $p !~ /^Executing '.*':$/);
    do { $p = pop; } while (defined ($p) && $p !~ /^Execution of '.*' complete.$/);
    return @_;
}

sub compare_output {
    my ($expected) = pop @_;
    my ($output) = pop @_;
    my (%options) = @_;

    my (@output) = get_core_output (@$output);
    fail "'$test' didn't run or didn't produce any output\n" if !@output;

    if (exists $options{IGNORE_EXIT_CODES}) {
	delete $options{IGNORE_EXIT_CODES};
	@output = grep (!/^[a-zA-Z0-9-_]+: exit\(\d+\)$/, @output);
    }
    die "unknown option " . (keys (%options))[0] . "\n" if %options;

    my ($msg);

    # Compare actual output against each allowed output.
    foreach my $exp_string (@$expected) {
	my (@expected) = split ("\n", $exp_string);

	$msg .= "Acceptable output:\n";
	$msg .= join ('', map ("  $_\n", @expected));

	# Check whether actual and expected match.
	# If it's a perfect match, we're done.
	if ($#output == $#expected) {
	    my ($eq) = 1;
	    for (my ($i) = 0; $i <= $#expected; $i++) {
		$eq = 0 if $output[$i] ne $expected[$i];
	    }
	    pass if $eq;
	}

	# They differ.  Output a diff.
	my (@diff) = "";
	my ($d) = Algorithm::Diff->new (\@expected, \@output);
	while ($d->Next ()) {
	    my ($ef, $el, $af, $al) = $d->Get (qw (min1 max1 min2 max2));
	    if ($d->Same ()) {
		push (@diff, map ("  $_\n", $d->Items (1)));
	    } else {
		push (@diff, map ("- $_\n", $d->Items (1))) if $d->Items (1);
		push (@diff, map ("+ $_\n", $d->Items (2))) if $d->Items (2);
	    }
	}

	$msg .= "Differences in `diff -u' format:\n";
	$msg .= join ('', @diff);
    }

    # Failed to match.  Report failure.
    fail "Test output failed to match any acceptable form.\n\n$msg";
}

sub fail {
    finish ("FAIL", @_);
}

sub pass {
    finish ("PASS", @_);
}

sub finish {
    my ($verdict, @rest) = @_;

    my ($result_fn) = "$test.result";
    open (RESULT, '>', $result_fn) or die "$result_fn: create: $!\n";
    print RESULT "$verdict\n", $messages, @rest;
    close (RESULT);

    if ($verdict eq 'PASS') {
	print STDOUT "pass $test\n";
    } else {
	print STDOUT "FAIL $test\n";
    }
    print STDOUT $messages, @rest, "\n";

    exit 0;
}

sub read_text_file {
    my ($file_name) = @_;
    open (FILE, '<', $file_name) or die "$file_name: open: $!\n";
    my (@content) = <FILE>;
    chomp (@content);
    close (FILE);
    return @content;
}

1;
