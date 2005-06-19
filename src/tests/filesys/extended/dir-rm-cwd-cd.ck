# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected (IGNORE_EXIT_CODES => 1, [<<'EOF', <<'EOF']);
(dir-rm-cwd-cd) begin
(dir-rm-cwd-cd) mkdir "a"
(dir-rm-cwd-cd) chdir "a"
(dir-rm-cwd-cd) remove "/a" (must not crash)
(dir-rm-cwd-cd) chdir "/a" (remove succeeded so this must return false)
(dir-rm-cwd-cd) end
EOF
(dir-rm-cwd-cd) begin
(dir-rm-cwd-cd) mkdir "a"
(dir-rm-cwd-cd) chdir "a"
(dir-rm-cwd-cd) remove "/a" (must not crash)
(dir-rm-cwd-cd) chdir "/a" (remove failed so this must succeed)
(dir-rm-cwd-cd) end
EOF
