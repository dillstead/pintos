use strict;
use warnings;

our ($test);

our ($GRADES_DIR);
our ($verbose);
our (%args);

use Getopt::Long;
use POSIX;

# Source tarballs.

# Extracts the group's source files into pintos/src,
# applies any patches providing in the grading directory,
# and installs a default pintos/src/constants.h
sub obtain_sources {
    # Nothing to do if we already have a source tree.
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
    # probably be named 00-debug.patch, 01-bitmap.patch, etc.
    # Filenames in patches should be in the format pintos/src/...
    print "Patching...\n";
    for my $patch (glob ("$GRADES_DIR/patches/*.patch")) {
	my ($stem);
	($stem = $patch) =~ s%^$GRADES_DIR/patches/%% or die;
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

	print "Multiple tarballs:";
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

# Compiling.

sub compile {
    print "Compiling...\n";
    xsystem ("cd pintos/src/filesys && make", LOG => "make")
	or return "compile error";
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

    # Run the test.
    my ($result) = run_test ($test);

    # Save the results for later.
    open (DONE, ">$cache_file") or die "$cache_file: create: $!\n";
    print DONE "$result\n";
    close (DONE);

    return $result;
}

# Creates an output directory for the test,
# creates all the files needed 
sub run_test {
    # Make output directory.
    mkdir "output/$test";

    my ($fs_size) = $test ne 'grow-too-big' ? 2 : .25;
    xsystem ("pintos make-disk output/$test/fs.dsk $fs_size >/dev/null 2>&1",
	     DIE => "failed to create file system disk");
    xsystem ("pintos make-disk output/$test/swap.dsk 2 >/dev/null 2>&1",
	     DIE => "failed to create swap disk");

    # Format disk, install test.
    my ($pintos_base_cmd) =
	"pintos "
	. "--os-disk=pintos/src/filesys/build/os.dsk "
	. "--fs-disk=output/$test/fs.dsk "
	. "--swap-disk=output/$test/swap.dsk "
	. "-v";
    unlink ("output/$test/fs.dsk", "output/$test/swap.dsk"),
    return "format/put error" 
	if xsystem ("$pintos_base_cmd put -f $GRADES_DIR/$test $test",
		    LOG => "$test/put", TIMEOUT => 60, EXPECT => 1) ne 'ok';

    my (@extra_files);
    push (@extra_files, "child-syn-read") if $test eq 'syn-read';
    push (@extra_files, "child-syn-wrt") if $test eq 'syn-write';
    push (@extra_files, "child-syn-rw") if $test eq 'syn-rw';
    for my $fn (@extra_files) {
	return "format/put error" 
	    if xsystem ("$pintos_base_cmd put $GRADES_DIR/$fn $fn",
			LOG => "$test/put-$fn", TIMEOUT => 60, EXPECT => 1)
               ne 'ok';
    }
    
    # Run.
    my ($timeout) = 120;
    my ($testargs) = defined ($args{$test}) ? " $args{$test}" : "";
    my ($retval) =
	xsystem ("$pintos_base_cmd run -q -ex \"$test$testargs\"",
		 LOG => "$test/run", TIMEOUT => $timeout, EXPECT => 1);
    my ($result);
    if ($retval eq 'ok') {
	$result = "ok";
    } elsif ($retval eq 'timeout') {
	$result = "Timed out after $timeout seconds";
    } elsif ($retval eq 'error') {
	$result = "Bochs error";
    } else {
	die;
    }
    unlink ("output/$test/fs.dsk", "output/$test/swap.dsk");
    return $result;
}

# Grade the test.
sub grade_test {
    # Read test output.
    my (@output) = snarf ("output/$test/run.out");

    # If there's a function "grade_$test", use it to evaluate the output.
    # If there's a file "$GRADES_DIR/$test.exp", compare its contents
    # against the output.
    # (If both exist, prefer the function.)
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

sub c {
    print "$test\n";
}

1;
