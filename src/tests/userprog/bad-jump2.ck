# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(bad-jump2) begin
bad-jump2: exit(-1)
EOF
