use strict;
use warnings;

our ($test);

our ($GRADES_DIR);
our ($verbose);
our %result;
our %details;
our %extra;
our @TESTS;
our $action;
our $hw;

use POSIX;
use Getopt::Long qw(:config no_ignore_case);
use Algorithm::Diff;

sub parse_cmd_line {
    my ($do_regex, $no_regex);
    GetOptions ("v|verbose+" => \$verbose,
		"h|help" => sub { usage (0) },
		"d|do-tests=s" => \$do_regex,
		"n|no-tests=s" => \$no_regex,
		"c|clean" => sub { set_action ('clean'); },
		"x|extract" => sub { set_action ('extract'); },
		"b|build" => sub { set_action ('build'); },
		"t|test" => sub { set_action ('test'); },
		"a|assemble" => sub { set_action ('assemble'); })
	or die "Malformed command line; use --help for help.\n";
    die "Non-option arguments not supported; use --help for help.\n"
	if @ARGV > 0;
    @TESTS = split(/,/, join (',', @TESTS)) if defined @TESTS;

    if (!defined $action) {
	$action = -e 'review.txt' ? 'assemble' : 'test';
    }

    my (@default_tests) = @_;
    @TESTS = @default_tests;
    @TESTS = grep (/$do_regex/, @TESTS) if defined $do_regex;
    @TESTS = grep (!/$no_regex/, @TESTS) if defined $no_regex;
}

sub set_action {
    my ($new_action) = @_;
    die "actions `$action' and `$new_action' conflict\n"
	if defined ($action) && $action ne $new_action;
    $action = $new_action;
}

sub usage {
    my ($exitcode) = @_;
    print <<EOF;
run-tests, for grading Pintos $hw projects.

Invoke from a directory containing a student tarball named by
the submit script, e.g. username.MMM.DD.YY.hh.mm.ss.tar.gz.

Workflow:

1. Extracts the source tree into pintos/src and applies patches.

2. Builds the source tree.  (The threads project modifies and rebuilds
   the source tree for every test.)

3. Runs the tests on the source tree and grades them.  Writes
   "tests.out" with a summary of the test results, and "details.out"
   with test failure and warning details.

4. By hand, copy "review.txt" from the tests directory and use it as a
   template for grading design documents.

5. Assembles "grade.txt", "tests.out", "review.txt", and "tests.out"
   into "grade.out".  This is primarily simple concatenation, but
   point totals are tallied up as well.

Options:
  -c, --clean        Delete test results and temporary files, then exit.
  -d, --do-tests=RE  Run only tests that match the given regular expression.
  -n, --no-tests=RE  Do not run tests that match the given regular expression.
  -x, --extract      Stop after step 1.
  -b, --build        Stop after step 2.
  -t, --test         Stop after step 3 (default if "review.txt" not present).
  -a, --assemble     Stop after step 5 (default if "review.txt" exists).
  -v, --verbose      Print command lines of subcommands before executing them.
  -h, --help         Print this help message.
EOF
    exit $exitcode;
}

# Source tarballs.

# Extracts the group's source files into pintos/src,
# applies any patches providing in the grading directory,
# and installs a default pintos/src/constants.h
sub extract_sources {
    # Make sure the output dir exists.
    -d ("output") || mkdir ("output") or die "output: mkdir: $!\n";

    # Nothing else to do if we already have a source tree.
    return if -d "pintos";

    my ($tarball) = choose_tarball ();

    # Extract sources.
    print "Creating pintos/src...\n";
    mkdir "pintos" or die "pintos: mkdir: $!\n";
    mkdir "pintos/src" or die "pintos/src: mkdir: $!\n";

    print "Extracting $tarball into pintos/src...\n";
    xsystem ("cd pintos/src && tar xzf ../../$tarball",
	     DIE => "extraction failed\n");

    # Run custom script for this submission, if provided.
    if (-e "fixme.sh") {
	print "Running fixme.sh...\n";
	xsystem ("sh -e fixme.sh", DIE => "fix script failed\n");
    } else {
	print "No fixme.sh, assuming no custom changes needed.\n";
    }

    # Apply patches from grading directory.
    # Patches are applied in lexicographic order, so they should
    # probably be named 00debug.patch, 01bitmap.patch, etc.
    # Filenames in patches should be in the format pintos/src/...
    print "Patching...\n";
    for my $patch (glob ("$GRADES_DIR/patches/*.patch")) {
	my ($stem);
	($stem = $patch) =~ s%^$GRADES_DIR/patches/%% or die;
	print "Applying $patch...\n";
	xsystem ("patch -fs -p0 < $patch",
		 LOG => $stem, DIE => "applying patch $stem failed\n");
    }

    # Install default pintos/src/constants.h.
    open (CONSTANTS, ">pintos/src/constants.h")
	or die "constants.h: create: $!\n";
    print CONSTANTS "#define THREAD_JOIN_IMPLEMENTED 1\n";
    close CONSTANTS;
}

# Returns the name of the tarball to extract.
sub choose_tarball {
    my (@tarballs)
	= grep (/^[a-z0-9]+\.[A-Za-z]+\.\d+\.\d+\.\d+\.\d+.\d+\.tar\.gz$/,
		glob ("*.tar.gz"));
    die "no pintos dir, no files matching username.MMM.DD.YY.hh.mm.ss.tar.gz\n"
	if scalar (@tarballs) == 0;

    if (@tarballs > 1) {
	# Sort tarballs in order by time.
	@tarballs = sort { ext_mdyHMS ($a) cmp ext_mdyHMS ($b) } @tarballs;

	print "Multiple tarballs:\n";
	print "\t$_ submitted ", ext_mdyHMS ($_), "\n" foreach @tarballs;
	print "Choosing $tarballs[$#tarballs]\n";
    }
    return $tarballs[$#tarballs];
}

# Extract the date within a tarball name into a string that compares
# lexicographically in chronological order.
sub ext_mdyHMS {
    my ($s) = @_;
    my ($ms, $d, $y, $H, $M, $S) =
	$s =~ /.([A-Za-z]+)\.(\d+)\.(\d+)\.(\d+)\.(\d+).(\d+)\.tar\.gz$/
	or die;
    my ($m) = index ("janfebmaraprmayjunjulaugsepoctnovdec", lc $ms) / 3
	or die;
    return sprintf "%02d-%02d-%02d %02d:%02d:%02d", $y, $m, $d, $H, $M, $S;
}

# Building.

sub build {
    print "Compiling...\n";
    xsystem ("cd pintos/src/$hw && make", LOG => "make") eq 'ok'
	or return "Build error";
}

# Run and grade the tests.
sub run_and_grade_tests {
    for $test (@TESTS) {
	print "$test: ";
	my ($result) = get_test_result ();
	chomp ($result);

	my ($grade) = grade_test ($test);
	chomp ($grade);
	
	my ($msg) = $result eq 'ok' ? $grade : "$result - $grade";
	$msg .= " - with warnings"
	    if $grade eq 'ok' && defined $details{$test};
	print "$msg\n";
	
	$result{$test} = $grade;
    }
}

# Write test grades to tests.out.
sub write_grades {
    my (@summary) = snarf ("$GRADES_DIR/tests.txt");

    my ($ploss) = 0;
    my ($tloss) = 0;
    my ($total) = 0;
    my ($warnings) = 0;
    for (my ($i) = 0; $i <= $#summary; $i++) {
	local ($_) = $summary[$i];
	if (my ($loss, $test) = /^  -(\d+) ([-a-zA-Z0-9]+):/) {
	    my ($result) = $result{$test} || "Not tested.";

	    if ($result eq 'ok') {
		if (!defined $details{$test}) {
		    # Test successful and no warnings.
		    splice (@summary, $i, 1);
		    $i--;
		} else {
		    # Test successful with warnings.
		    s/-(\d+) //;
		    $summary[$i] = $_;
		    splice (@summary, $i + 1, 0,
			    "     Test passed with warnings.  "
			    . "Details at end of file.");
		    $warnings++;
		} 
	    } else {
		$ploss += $loss;
		$tloss += $loss;
		splice (@summary, $i + 1, 0,
			map ("     $_", split ("\n", $result)));
	    }
	} elsif (my ($ptotal) = /^Score: \/(\d+)$/) {
	    $total += $ptotal;
	    $summary[$i] = "Score: " . ($ptotal - $ploss) . "/$ptotal";
	    splice (@summary, $i, 0, "  All tests passed.")
		if $ploss == 0 && !$warnings;
	    $ploss = 0;
	    $warnings = 0;
	    $i++;
	}
    }
    my ($ts) = "(" . ($total - $tloss) . "/" . $total . ")";
    $summary[0] =~ s/\[\[total\]\]/$ts/;

    open (SUMMARY, ">tests.out");
    print SUMMARY map ("$_\n", @summary);
    close (SUMMARY);
}

# Write failure and warning details to details.out.
sub write_details {
    open (DETAILS, ">details.out");
    my ($n) = 0;
    for $test (@TESTS) {
	next if $result{$test} eq 'ok' && !defined $details{$test};
	
	my ($details) = $details{$test};
	next if !defined ($details) && ! -e "output/$test/run.out";

	my ($banner);
	if ($result{$test} ne 'ok') {
	    $banner = "$test failure details"; 
	} else {
	    $banner = "$test warnings";
	}

	print DETAILS "\n" if $n++;
	print DETAILS "--- $banner ", '-' x (50 - length ($banner));
	print DETAILS "\n\n";

	if (!defined $details) {
	    my (@output) = snarf ("output/$test/run.out");

	    # Print only the first in a series of recursing panics.
	    my ($panic) = 0;
	    for my $i (0...$#output) {
		local ($_) = $output[$i];
		if (/PANIC/ && $panic++ > 0) {
		    @output = @output[0...$i];
		    push (@output,
			  "[...details of recursive panic(s) omitted...]");
		    last;
		}
	    }
	    $details = "Output:\n\n" . join ('', map ("$_\n", @output));
	}
	print DETAILS $details;

	print DETAILS "\n", "-" x 10, "\n\n$extra{$test}"
	    if defined $extra{$test};
    }
    close (DETAILS);
}

sub xsystem {
    my ($command, %options) = @_;
    print "$command\n" if $verbose || $options{VERBOSE};

    my ($log) = $options{LOG};

    my ($pid, $status);
    eval {
	local $SIG{ALRM} = sub { die "alarm\n" };
	alarm $options{TIMEOUT} if defined $options{TIMEOUT};
	$pid = fork;
	die "fork: $!\n" if !defined $pid;
	if (!$pid) {
	    if (defined $log) {
		open (STDOUT, ">output/$log.out");
		open (STDERR, ">output/$log.err");
	    }
	    exec ($command);
	    exit (-1);
	}
	waitpid ($pid, 0);
	$status = $?;
	alarm 0;
    };

    my ($result);
    if ($@) {
	die unless $@ eq "alarm\n";   # propagate unexpected errors
	my ($i);
	for ($i = 0; $i < 10; $i++) {
	    kill ('SIGTERM', $pid);
	    sleep (1);
	    my ($retval) = waitpid ($pid, WNOHANG);
	    last if $retval == $pid || $retval == -1;
	    print "Timed out - Waiting for $pid to die" if $i == 0;
	    print ".";
	}
	print "\n" if $i;
	$result = 'timeout';
    } else {
	if (WIFSIGNALED ($status)) {
	    my ($signal) = WTERMSIG ($status);
	    die "Interrupted\n" if $signal == SIGINT;
	    print "Child terminated with signal $signal\n";
	}

	my ($exp_status) = !defined ($options{EXPECT}) ? 0 : $options{EXPECT};
	$result = WIFEXITED ($status) && WEXITSTATUS ($status) == $exp_status
	    ? "ok" : "error";
    }


    if ($result eq 'error' && defined $options{DIE}) {
	my ($msg) = $options{DIE};
	if (defined ($log)) {
	    chomp ($msg);
	    $msg .= "; see output/$log.err and output/$log.out for details\n";
	}
	die $msg;
    } elsif ($result ne 'error' && defined ($log)) {
	unlink ("output/$log.err");
    }

    return $result;
}

sub get_test_result {
    my ($cache_file) = "output/$test/run.result";
    # Reuse older results if any.
    if (open (RESULT, "<$cache_file")) {
	my ($result);
	$result = <RESULT>;
	chomp $result;
	close (RESULT);
	return $result;
    }

    # If there's residue from an earlier test, move it to .old.
    # If there's already a .old, delete it.
    xsystem ("rm -rf output/$test.old", VERBOSE => 1) if -d "output/$test.old";
    rename "output/$test", "output/$test.old" or die "rename: $!\n"
	if -d "output/$test";

    # Make output directory.
    mkdir "output/$test";

    # Run the test.
    my ($result) = run_test ($test);

    # Delete any disks in the output directory because they take up
    # lots of space.
    unlink (glob ("output/$test/*.dsk"));

    # Save the results for later.
    open (DONE, ">$cache_file") or die "$cache_file: create: $!\n";
    print DONE "$result\n";
    close (DONE);

    return $result;
}

sub run_pintos {
    my ($cmd_line, %args) = @_;
    $args{EXPECT} = 1 unless defined $args{EXPECT};
    my ($retval) = xsystem ($cmd_line, %args);
    return 'ok' if $retval eq 'ok';
    return "Timed out after $args{TIMEOUT} seconds" if $retval eq 'timeout';
    return 'Error running Bochs' if $retval eq 'error';
    die;
}

# Grade the test.
sub grade_test {
    # Read test output.
    my (@output) = snarf ("output/$test/run.out");

    # If there's a function "grade_$test", use it to evaluate the output.
    # If there's a file "$GRADES_DIR/$test.exp", compare its contents
    # against the output.
    # (If both exist, prefer the function.)
    #
    # If the test was successful, it returns normally.
    # If it failed, it invokes `die' with an error message terminated
    # by a new-line.  The message will be given as an explanation in
    # the output file tests.out.
    # (Internal errors will invoke `die' without a terminating
    # new-line, in which case we detect it and propagate the `die'
    # upward.)
    my ($grade_func) = "grade_$test";
    $grade_func =~ s/-/_/g;
    if (-e "$GRADES_DIR/$test.exp" && !defined (&$grade_func)) {
	eval {
	    verify_common (@output);
	    compare_output ("$GRADES_DIR/$test.exp", @output);
	}
    } else {
	eval "$grade_func (\@output)";
    }
    if ($@) {
	die $@ if $@ =~ /at \S+ line \d+$/;
	return $@;
    }
    return "ok";
}

# Do final grade.
# Combines grade.txt, tests.out, review.txt, and details.out,
# producing grade.out.
sub assemble_final_grade {
    open (OUT, ">grade.out") or die "grade.out: create: $!\n";

    open (GRADE, "<grade.txt") or die "grade.txt: open: $!\n";
    while (<GRADE>) {
	last if /^\s*$/;
	print OUT;
    }
    close (GRADE);
    
    my (@tests) = snarf ("tests.out");
    my ($p_got, $p_pos) = $tests[0] =~ m%\((\d+)/(\d+)\)% or die;

    my (@review) = snarf ("review.txt");
    my ($part_lost) = (0, 0);
    for (my ($i) = $#review; $i >= 0; $i--) {
	local ($_) = $review[$i];
	if (my ($loss) = /^\s*([-+]\d+)/) {
	    $part_lost += $loss;
	} elsif (my ($out_of) = m%\[\[/(\d+)\]\]%) {
	    my ($got) = $out_of + $part_lost;
	    $got = 0 if $got < 0;
	    $review[$i] =~ s%\[\[/\d+\]\]%($got/$out_of)% or die;
	    $part_lost = 0;

	    $p_got += $got;
	    $p_pos += $out_of;
	}
    }
    die "Lost points outside a section\n" if $part_lost;

    for (my ($i) = 1; $i <= $#review; $i++) {
	if ($review[$i] =~ /^-{3,}\s*$/ && $review[$i - 1] !~ /^\s*$/) {
	    $review[$i] = '-' x (length ($review[$i - 1]));
	}
    }

    print OUT "\nOVERALL SCORE\n";
    print OUT "-------------\n";
    print OUT "$p_got points out of $p_pos total\n\n";

    print OUT map ("$_\n", @tests), "\n";
    print OUT map ("$_\n", @review), "\n";

    print OUT "DETAILS\n";
    print OUT "-------\n\n";
    print OUT map ("$_\n", snarf ("details.out"));
}

# Clean up our generated files.
sub clean_dir {
    # Verify that we're roughly in the correct directory
    # before we go blasting away files.
    choose_tarball ();

    # Blow away everything.
    xsystem ("rm -rf output pintos", VERBOSE => 1);
    xsystem ("rm -f details.out tests.out", VERBOSE => 1);
}

# Provided a test's output as an array, verifies that it, in general,
# looks sensible; that is, that there are no PANIC or FAIL messages,
# that Pintos started up and shut down normally, and so on.
# Die if something odd found.
sub verify_common {
    my (@output) = @_;

    die "No output at all\n" if @output == 0;

    look_for_panic (@output);
    look_for_fail (@output);
    look_for_triple_fault (@output);
    
    die "Didn't start up properly: no \"Pintos booting\" startup message\n"
	if !grep (/Pintos booting with.*kB RAM\.\.\./, @output);
    die "Didn't start up properly: no \"Boot complete\" startup message\n"
	if !grep (/Boot complete/, @output);
    die "Didn't shut down properly: no \"Timer: # ticks\" shutdown message\n"
        if !grep (/Timer: \d+ ticks/, @output);
    die "Didn't shut down properly: no \"Powering off\" shutdown message\n"
	if !grep (/Powering off/, @output);
}

sub look_for_panic {
    my (@output) = @_;

    my ($panic) = grep (/PANIC/, @output);
    return unless defined $panic;

    my ($details) = "Kernel panic:\n  $panic\n";

    my (@stack_line) = grep (/Call stack:/, @output);
    if (@stack_line != 0) {
	$details .= "  $stack_line[0]\n\n";
	$details .= "Translation of backtrace:\n";
	my (@addrs) = $stack_line[0] =~ /Call stack:((?: 0x[0-9a-f]+)+)/;

	my ($A2L);
	if (`uname -m`
	    =~ /i.86|pentium.*|[pk][56]|nexgen|viac3|6x86|athlon.*/) {
	    $A2L = "addr2line";
	} else {
	    $A2L = "i386-elf-addr2line";
	}
	my ($kernel_o);
	if ($hw eq 'threads') {
	    $kernel_o = "output/$test/kernel.o";
	} else {
	    $kernel_o = "pintos/src/$hw/build/kernel.o";
	}
	open (A2L, "$A2L -fe $kernel_o @addrs|");
	for (;;) {
	    my ($function, $line);
	    last unless defined ($function = <A2L>);
	    $line = <A2L>;
	    chomp $function;
	    chomp $line;
	    $details .= "  $function ($line)\n";
	}
    }

    if ($panic =~ /sec_no < d->capacity/) {
	$details .= <<EOF;
\nThis assertion commonly fails when accessing a file via an inode that
has been closed and freed.  Freeing an inode clears all its sector
indexes to 0xcccccccc, which is not a valid sector number for disks
smaller than about 1.6 TB.
EOF
	}

    $extra{$test} = $details;
    die "Kernel panic.  Details at end of file.\n";
}

sub look_for_fail {
    my (@output) = @_;
    
    my ($failure) = grep (/FAIL/, @output);
    return unless defined $failure;

    # Eliminate uninteresting header and trailer info if possible.
    # The `eval' catches the `die' from get_core_output() in the "not
    # possible" case.
    eval {
	my (@core) = get_core_output (@output);
	$details{$test} = "Program output:\n\n" . join ('', map ("$_\n", @core));
    };

    # Most output lines are prefixed by (test-name).  Eliminate this
    # from our `die' message for brevity.
    $failure =~ s/^\([^\)]+\)\s+//;
    die "$failure.  Details at end of file.\n";
}

sub look_for_triple_fault {
    my (@output) = @_;

    return unless grep (/Pintos booting/, @output) > 1;

    my ($details) = <<EOF;
Pintos spontaneously rebooted during this test.  This is most often
due to unhandled page faults.  Output from initial boot through the
first reboot is shown below:

EOF

    my ($i) = 0;
    local ($_);
    for (@output) {
	$details .= "  $_\n";
	last if /Pintos booting/ && ++$i > 1;
    }
    $details{$test} = $details;
    die "Triple-fault caused spontaneous reboot(s).  "
	. "Details at end of file.\n";
}

# Get @output without header or trailer.
# Die if not possible.
sub get_core_output {
    my (@output) = @_;

    my ($first);
    for ($first = 0; $first <= $#output; $first++) {
	my ($line) = $output[$first];
	$first++, last
	    if ($hw ne 'threads' && $line =~ /^Executing '$test.*':$/)
	    || ($hw eq 'threads'
		&& grep (/^Boot complete.$/, @output[0...$first - 1])
		&& $line =~ /^\s*$/);
    }

    my ($last);
    for ($last = $#output; $last >= 0; $last--) {
	$last--, last if $output[$last] =~ /^Timer: \d+ ticks$/;
    }

    if ($last < $first) {
	my ($no_first) = $first > $#output;
	my ($no_last) = $last < $#output;
	die "Couldn't locate output.\n";
    }

    return @output[$first ... $last];
}

sub canonicalize_exit_codes {
    my (@output) = @_;

    # Exit codes are supposed to be printed in the form "process: exit(code)"
    # but people get unfortunately creative with it.
    for my $i (0...$#output) {
	local ($_) = $output[$i];
	
	my ($process, $code);
	if ((($process, $code) = /^([-a-z0-9 ]+):.*[ \(](-?\d+)\b\)?$/)
	    || (($process, $code) = /^([-a-z0-9 ]+) exit\((-?\d+)\)$/)
	    || (($process, $code)
		= /^([-a-z0-9 ]+) \(.*\): exit\((-?\d+)\)$/)
	    || (($process, $code) = /^([-a-z0-9 ]+):\( (-?\d+) \) $/)
	    || (($code, $process) = /^shell: exit\((-?\d+)\) \| ([-a-z0-9]+)/))
	{
	    # We additionally truncate to 15 character and strip all
	    # but the first word.
	    $process = substr ($process, 0, 15);
	    $process =~ s/\s.*//;
	    $output[$i] = "$process: exit($code)\n";
	}
    }

    return @output;
}

sub strip_exit_codes {
    return grep (!/^[-a-z0-9]+: exit\(-?\d+\)/, canonicalize_exit_codes (@_));
}

sub compare_output {
    my ($exp, @actual) = @_;

    # Canonicalize output for comparison.
    @actual = get_core_output (map ("$_\n", @actual));
    if ($hw eq 'userprog') {
	@actual = canonicalize_exit_codes (@actual);
    } elsif ($hw eq 'vm' || $hw eq 'filesys') {
	@actual = strip_exit_codes (@actual);
    }

    # There *was* some output, right?
    die "Program produced no output.\n" if !@actual;

    # Read expected output.
    my (@exp) = map ("$_\n", snarf ($exp));

    # Pessimistically, start preparation of detailed failure message.
    my ($details) = "";
    $details .= "$test actual output:\n";
    $details .= join ('', map ("  $_", @actual));

    # Set true when we find expected output that matches our actual
    # output except for some extra actual output (that doesn't seem to
    # be an error message etc.).
    my ($fuzzy_match) = 0;

    # Compare actual output against each allowed output.
    while (@exp != 0) {
	# Grab one set of allowed output from @exp into @expected.
	my (@expected);
	while (@exp != 0) {
	    my ($s) = shift (@exp);
	    last if $s eq "--OR--\n";
	    push (@expected, $s);
	}

	$details .= "\n$test acceptable output:\n";
	$details .= join ('', map ("  $_", @expected));

	# Check whether actual and expected match.
	# If it's a perfect match, return.
	if ($#actual == $#expected) {
	    my ($eq) = 1;
	    for (my ($i) = 0; $i <= $#expected; $i++) {
		$eq = 0 if $actual[$i] ne $expected[$i];
	    }
	    return if $eq;
	}

	# They differ.  Output a diff.
	my (@diff) = "";
	my ($d) = Algorithm::Diff->new (\@expected, \@actual);
	my ($not_fuzzy_match) = 0;
	while ($d->Next ()) {
	    my ($ef, $el, $af, $al) = $d->Get (qw (min1 max1 min2 max2));
	    if ($d->Same ()) {
		push (@diff, map ("  $_", $d->Items (1)));
	    } else {
		push (@diff, map ("- $_", $d->Items (1))) if $d->Items (1);
		push (@diff, map ("+ $_", $d->Items (2))) if $d->Items (2);
		if ($d->Items (1)
		    || grep (/\($test\)|exit\(-?\d+\)|dying due to|Page fault/,
			     $d->Items (2))) {
		    $not_fuzzy_match = 1;
		}
	    }
	}

	# If we didn't find anything that means it's not,
	# it's a fuzzy match.
	$fuzzy_match = 1 if !$not_fuzzy_match;

	$details .= "Differences in `diff -u' format:\n";
	$details .= join ('', @diff);
	$details .= "(This is considered a `fuzzy match'.)\n"
	    if !$not_fuzzy_match;
    }

    # Failed to match.  Report failure.
    if ($fuzzy_match) {
	$details =
	    "This test passed, but with extra, unexpected output.\n"
	    . "Please inspect your code to make sure that it does not\n"
	    . "produce output other than as specified in the project\n"
	    . "description.\n\n"
	    . "$details";
    } else {
	$details =
	    "This test failed because its output did not match any\n"
	    . "of the acceptable form(s).\n\n"
	    . "$details";
    }

    $details{$test} = $details;
    die "Output differs from expected.  Details at end of file.\n"
	unless $fuzzy_match;
}

# Reads and returns the contents of the specified file.
# In an array context, returns the file's contents as an array of
# lines, omitting new-lines.
# In a scalar context, returns the file's contents as a single string.
sub snarf {
    my ($file) = @_;
    open (OUTPUT, $file) or die "$file: open: $!\n";
    my (@lines) = <OUTPUT>;
    chomp (@lines);
    close (OUTPUT);
    return wantarray ? @lines : join ('', map ("$_\n", @lines));
}

# Returns true if the two specified files are byte-for-byte identical,
# false otherwise.
sub files_equal {
    my ($a, $b) = @_;
    my ($equal);
    open (A, "<$a") or die "$a: open: $!\n";
    open (B, "<$b") or die "$b: open: $!\n";
    if (-s A != -s B) {
	$equal = 0;
    } else {
	my ($sa, $sb);
	for (;;) {
	    sysread (A, $sa, 1024);
	    sysread (B, $sb, 1024);
	    $equal = 0, last if $sa ne $sb;
	    $equal = 1, last if $sa eq '';
	}
    }
    close (A);
    close (B);
    return $equal;
}

# Returns true if the specified file is byte-for-byte identical with
# the specified string.
sub file_contains {
    my ($file, $expected) = @_;
    open (FILE, "<$file") or die "$file: open: $!\n";
    my ($actual);
    sysread (FILE, $actual, -s FILE);
    my ($equal) = $actual eq $expected;
    close (FILE);
    return $equal;
}

1;
