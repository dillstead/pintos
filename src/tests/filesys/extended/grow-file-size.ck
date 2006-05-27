# -*- perl -*-
use strict;
use warnings;
use tests::tests;
use tests::random;
check_expected (IGNORE_EXIT_CODES => 1, [<<'EOF']);
(grow-file-size) begin
(grow-file-size) create "testfile"
(grow-file-size) open "testfile"
(grow-file-size) writing "testfile"
(grow-file-size) close "testfile"
(grow-file-size) open "testfile" for verification
(grow-file-size) verified contents of "testfile"
(grow-file-size) close "testfile"
(grow-file-size) end
EOF
my ($string);
check_archive ({"testfile" => [random_bytes (2134)]});
pass;
