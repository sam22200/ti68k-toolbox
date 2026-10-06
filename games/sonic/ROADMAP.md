# Sonic: bounded milestones

Only the first five original view widths of GHZ1 are in scope. Grilling is
complete: half scale, traversal first, rigid bridge, full-height camera and
restart on falls; finish marker at original X1536.

| Step | Deliverable | Acceptance | Status |
| --- | --- | --- | --- |
| 0 | Reproducible original boot, direct-play state, headless RAM/input/image tools | Real movement/jump and deterministic state replay | Passed |
| 1 | Confirm port decisions; measure the next mechanic on original terrain | Recorded scale, first playable scope, reproducible numbers | Passed |
| 2 | C simulation: acceleration, braking, jump and rolling on controlled ground | Headless tests against observed rules and measured units | Passed |
| 3 | Real beginning of GHZ1: ground/slopes, camera and the necessary terrain rules | Scripted traversal of the agreed slice; PC/TI checks; cycle budget | Passed with diagnostic art |
| 4 | Rings, enemies and damage only as required by this slice | One test per mechanic and a repeatable traversal | Passed: 14 rings, Motobug/Buzz Bomber/Chopper, damage, recovery, PC/TI checks |
| 5 | ROM graphics, four greys, outlined Sonic, high contrast | One art review; benchmark and PC/TI checks | Passed: ROM scenery/actors, half-scale animation, white outlines; all headless checks and cycle budget |
| 6 | Calculator milestone validation | Titanium hardware checks after headless checks pass | Passed presentation/controls: real Titanium TiEmu GIF shows movement, jump, scroll, pickups, damage and bridge; real-time timing differs from the headless winning route |

Do not study later zones, menus, special stages, bosses, sound drivers or the
rest of the ROM until an explicit later milestone needs them. New loops,
springs, bridges or other mechanics are studied when the chosen terrain
requires them, not because they exist in Sonic 1.
