# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(mmap-zero) begin
(mmap-zero) create empty file "empty"
(mmap-zero) open "empty"
(mmap-zero) mmap "empty"
mmap-zero: exit(-1)
EOF
pass;
