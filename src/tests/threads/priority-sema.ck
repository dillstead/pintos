# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(priority-sema) begin
(priority-sema) Thread priority 32 woke up.
(priority-sema) Back in main thread.
(priority-sema) Thread priority 33 woke up.
(priority-sema) Back in main thread.
(priority-sema) Thread priority 34 woke up.
(priority-sema) Back in main thread.
(priority-sema) Thread priority 35 woke up.
(priority-sema) Back in main thread.
(priority-sema) Thread priority 36 woke up.
(priority-sema) Back in main thread.
(priority-sema) Thread priority 37 woke up.
(priority-sema) Back in main thread.
(priority-sema) Thread priority 38 woke up.
(priority-sema) Back in main thread.
(priority-sema) Thread priority 39 woke up.
(priority-sema) Back in main thread.
(priority-sema) Thread priority 40 woke up.
(priority-sema) Back in main thread.
(priority-sema) Thread priority 41 woke up.
(priority-sema) Back in main thread.
(priority-sema) end
EOF
