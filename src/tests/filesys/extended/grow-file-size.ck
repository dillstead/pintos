# -*- perl -*-
use strict;
use warnings;
use tests::tests;
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
