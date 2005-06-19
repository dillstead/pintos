# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected (IGNORE_EXIT_CODES => 1, [<<'EOF', <<'EOF']);
(dir-open) begin
(dir-open) mkdir "xyzzy"
(dir-open) open "xyzzy"
(dir-open) open returned -1 -- ok
(dir-open) end
EOF
(dir-open) begin
(dir-open) mkdir "xyzzy"
(dir-open) open "xyzzy"
(dir-open) write "xyzzy" (must return -1, actually -1)
(dir-open) end
EOF
