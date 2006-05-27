# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected (IGNORE_EXIT_CODES => 1, [<<'EOF']);
(dir-rm-root) begin
(dir-rm-root) remove "/" (must fail)
(dir-rm-root) create "/a"
(dir-rm-root) end
EOF
check_archive ({"a" => ["\0" x 243]});
pass;
