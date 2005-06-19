# -*- perl -*-
use strict;
use warnings;
use tests::tests;
check_expected ([<<'EOF']);
(priority-condvar) begin
(priority-condvar) Thread priority 39 starting.
(priority-condvar) Thread priority 40 starting.
(priority-condvar) Thread priority 41 starting.
(priority-condvar) Thread priority 32 starting.
(priority-condvar) Thread priority 33 starting.
(priority-condvar) Thread priority 34 starting.
(priority-condvar) Thread priority 35 starting.
(priority-condvar) Thread priority 36 starting.
(priority-condvar) Thread priority 37 starting.
(priority-condvar) Thread priority 38 starting.
(priority-condvar) Signaling...
(priority-condvar) Thread priority 32 woke up.
(priority-condvar) Signaling...
(priority-condvar) Thread priority 33 woke up.
(priority-condvar) Signaling...
(priority-condvar) Thread priority 34 woke up.
(priority-condvar) Signaling...
(priority-condvar) Thread priority 35 woke up.
(priority-condvar) Signaling...
(priority-condvar) Thread priority 36 woke up.
(priority-condvar) Signaling...
(priority-condvar) Thread priority 37 woke up.
(priority-condvar) Signaling...
(priority-condvar) Thread priority 38 woke up.
(priority-condvar) Signaling...
(priority-condvar) Thread priority 39 woke up.
(priority-condvar) Signaling...
(priority-condvar) Thread priority 40 woke up.
(priority-condvar) Signaling...
(priority-condvar) Thread priority 41 woke up.
(priority-condvar) end
EOF
