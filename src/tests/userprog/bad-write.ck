# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(bad-write) begin
bad-write: exit(-1)
EOF
