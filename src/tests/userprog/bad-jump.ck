# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(bad-jump) begin
bad-jump: exit(-1)
EOF
