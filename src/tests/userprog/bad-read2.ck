# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(bad-read2) begin
bad-read2: exit(-1)
EOF
