# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected (IGNORE_EXIT_CODES => 1, [<<'EOF']);
(dir-empty-name) begin
(dir-empty-name) create "" (must return false)
(dir-empty-name) end
EOF
check_archive ({});
pass;
