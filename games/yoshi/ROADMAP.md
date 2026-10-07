# Yoshi's Island: bounded SNES experiment

Requested scope: the first five original SNES view widths of level 1-1,
starting directly in gameplay. The band is 1280 original pixels; the verified
endpoint is player X1264/Y1882, keeping the 16-pixel body inside the band.
The opening tutorial is a separate reference door, not the requested level.
Scale: one target pixel per two original pixels, as for Sonic. The terrain
milestone implements this band; interactions proceed through smaller playable
milestones. The user then requested ROM graphics before damage. This scope
belongs to this project, not the skill.

Grilling correction: half scale, camera and TI controls were provisional
implementation choices, not confirmed answers. Questions about TI commands,
faithful Baby Mario damage/recovery versus immediate restart, and original
egg aiming/rebounds versus a simplified throw have now been asked. The user
subsequently confirmed sweeping aim, L/R lock/resume and rebounds. TI control
mapping remains a stated choice. Damage is now explicitly
authorized with the crying baby and ten-second clock; the recharge question
remains a documented working assumption.

| Step | Deliverable | Acceptance | Status |
| --- | --- | --- | --- |
| 0 | Pinned headless original, cold boot and level 1-1 door | Real movement/jump, cartridge workspace and PPU exports, per-frame deterministic replay | Passed; disclosed tutorial-only position injection |
| 1 | Measure controlled movement, short/held jump and flutter | Local PAL variables confirmed by pokes; complete timelines and documented units | Passed for ordinary flat-ground/air movement: 20 controlled cases, 240 frames each |
| 2 | Portable C movement on controlled ground | PC tests against measured traces; TI state/screens equal; cycle budget | Passed: 4800 exact original states on PC and TI; five scenario screens equal; 91,066-cycle measured peak |
| 3 | Real terrain of the selected five-screen band | Collision format confirmed; camera and repeatable end-to-end traversal | Passed: actual Map16 terrain; 404 original collision states match PC/TI; 600-frame native replay agrees PC/TI, finish frame590; peak193552 cycles |
| 4 | Interactions required by that band | Complete the interaction milestones below | In progress |
| 4a | Tongue, Shy Guys and egg reserve | Original tongue/capture/swallow/spit timelines; eat two actors in a full native traversal; PC/TI state/screens and dense-frame budget | Passed: 2090 original action states and90 actor motion states; 800 native states PC=TI, two eggs at update692; initial diagnostic route peak198162, dense peak202432 cycles |
| 4b | Contact damage and Baby Mario | Enemy hit/recoil; outlined crying bubble; rescue by touch/tongue; initial ten-second clock, recharge and repeat hits; failure/reset; original observations plus native PC/TI checks and frame costs | Passed: 24 original launch states; ten-second clock; body/tongue rescue; 2098 complete PC/TI states,108 LCD checks; two hits/rescues in full traversal |
| 4c | Coins and complete Yoshi interactions | Animated collectable ROM coins, idle poses, visible tongue capture/retraction, world egg followers, aiming/throwing, stomp kills and capture/spit choice; headless mechanics, PC/TI state and pixels, dense cycle budget | Passed: 21 coins,174 directional frames; 2580 full native PC/TI states,325 LCD checks; dense peak209572 cycles, six-shot peak203716; plain21.5KB TI program. Provisional control: Diamond aims/throws; Shift captures/spits, Down converts |
| 4d | Remaining required actors | Study the census IDs and include collectibles/platforms needed by the route | Pending |
| 4c follow-up | Readable aiming/tongue and action preview | Visible outlined crosshair drawn after world actors, Alpha lock/unlock corresponding to original L/R, verified rebounds, GIF showing a complete jump/stomp and an egg hitting a live Maskass | Passed: PAL lock/resume and first floor rebound observed;15px cursor,2px outlined tongue,3225 complete PC/TI states,1031 LCD checks; dense209910/aim-dense209802 cycles; eight labeled preview clips; plain22937-byte program |
| 5 | ROM art in four greys | Readable outlined Yoshi/Baby Mario and Shy Guys; source comparison; native/dense frame costs | Passed: 876 original terrain cells verified; 58 directional frames; 2970 independent pixel checks; route peak204252/dense peak208934 cycles |
| 6 | Titanium hardware milestone | One clean run after headless tests pass | Pending |

The tutorial, menus, whole level, later levels, sound, transformations and
Super FX effects are not implementation targets unless this band requires them.

Milestone4a restores the five Shy Guys at their observed placements and adds
horizontal/upward tongue, capture, swallowing, automatic swallowing, spit
release and a six-egg reserve. Contact damage and Baby Mario are implemented
in milestone4b. The user's combined request establishes milestone4c: coins,
resting and ingestion poses, visible captured actors on the tongue, following
eggs and aimed throws, plus checking stomp and the capture/spit choice.
The graphic milestone was advanced ahead of damage at the user's request.
Milestone4b now follows the explicit request for the crying baby and ten
seconds. The new grilling question asks original rechargeable counter versus
a fresh fixed ten seconds on every hit. Pending an answer, the stated working
assumption is a rechargeable initial ten-second reserve, rescue by touch or
tongue, repeat hits preserving the remaining reserve, and ENTER after defeat.
Ten real seconds is a target adaptation: PAL drains its initial109 tenths
every four reference frames, faster than ten wall-clock seconds. Crying is
visual; sound remains outside the established scope. Existing movement and
interaction probe doors isolate their measured mechanics from new damage.
The user requested original sweeping aim with L/R lock/unlock and rebounds.
Diamond opens an oscillating reticle and Diamond again throws; Alpha toggles
lock/resume, retaining the world direction even if Yoshi turns. Down or Shift
cancels without consumption. The TI mapping is a stated implementation choice.
The new preview has eight focused mechanic clips, sampling every update of
the tongue, jump/stomp, lock/resume, distant enemy hit and floor rebound.
The current traversal
uses actual static terrain, with our camera and ground-following controller;
original slope-dependent velocities/impulses are not yet reproduced. Resolve
PAL scheduling before a real-time fidelity claim. Current ROM art flattens
background parallax and omits foreground BG3. Coins animate and collect;
shot hitboxes/rebounds, spatial egg following and pose timing use native models.
Next: remaining required actor census, PAL scheduling and hardware milestone.
