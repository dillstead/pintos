# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(priority-donate-nest) begin
(priority-donate-nest) Low thread should have priority 30.  Actual priority: 30.
(priority-donate-nest) Low thread should have priority 29.  Actual priority: 29.
(priority-donate-nest) Medium thread should have priority 29.  Actual priority: 29.
(priority-donate-nest) Medium thread got the lock.
(priority-donate-nest) High thread got the lock.
(priority-donate-nest) High thread finished.
(priority-donate-nest) High thread should have just finished.
(priority-donate-nest) Middle thread finished.
(priority-donate-nest) Medium thread should just have finished.
(priority-donate-nest) Low thread should have priority 31.  Actual priority: 31.
(priority-donate-nest) end
EOF
