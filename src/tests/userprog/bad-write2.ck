# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(bad-write2) begin
bad-write2: exit(-1)
EOF
