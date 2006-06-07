# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(bad-read) begin
bad-read: exit(-1)
EOF
