# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected (IGNORE_EXIT_CODES => 1, [<<'EOF']);
(dir-rm-cwd) begin
(dir-rm-cwd) mkdir "a"
(dir-rm-cwd) chdir "a"
(dir-rm-cwd) remove "/a" (must not crash)
(dir-rm-cwd) create "b" (must not crash)
(dir-rm-cwd) end
EOF
