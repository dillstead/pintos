# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(pt-grow-bad) begin
pt-grow-bad: exit(-1)
EOF
