# PC / TI campaign save exchange

`../tools/save_transfer.py` exchanges campaign saves in both directions. The PC
and TI already store identical, byte-addressed big-endian FSV records. The tool
only adds/removes the standard TI variable envelope and its `sav` trailer; no
stats, IDs, story flags or service state are converted or discarded. It works
offline and never contacts a calculator or emulator.

## PC to TI

From the repository root, export the PC session into player slot1:

```sh
rtk proxy python3 games/fft/tools/save_transfer.py to-ti games/fft/campaign/data/ffsave.sav /tmp/ffsave1.89y --slot 1
```

Transfer the resulting `.89y` with the normal link software while the game is
closed. Its internal variable is `main\ffsave1` regardless of the host filename.
Start `fftstory()` and choose **Continue**, slot1. Select an unused slot to keep
an existing game. The transfer tool itself neither sends files nor replaces a
calculator variable.

`--slot 2` / `3` select other player slots. `--slot session` exports the working
`ffsave` variable for test fixtures. Without `--slot`, the standard source names
`ffsave`, `ffsave1`, `ffsave2`, `ffsave3` are preserved; an arbitrary checkpoint
filename requires an explicit destination slot. Importing a session alone does
not create a player slot in the title's Continue list.

## TI to PC

Receive `ffsave1` through the link software as a `.89y`, then:

```sh
rtk proxy python3 games/fft/tools/save_transfer.py to-pc /tmp/ffsave1.89y games/fft/campaign/data
```

This creates `data/ffsave1.sav`. Start the PC campaign from that working directory
and choose **Continue**, slot1. A TI backup `.89g` with several saves requires
`--name ffsave1` to select the source variable. Add `--slot 3` to import it as
`ffsave3.sav` instead. No other slot, session, selection or configuration file is
changed. `ffslot` is only a remembered menu selection and need not be exchanged.

Both commands refuse an existing destination unless `--replace` is explicit.
They publish complete files and report the FSV version, byte count and SHA256
of the actual save payload. TI tables, lengths, checksums, variable type, trailer
and FSV/decoder bounds are checked before output. Gameplay validation and legacy
migration remain in the shared production `save_load_from` reader.

## Scope and compatibility

- FSV2..FSV9 files are preserved byte for byte. Both games perform their existing
  migration when loading older versions. Use builds supporting the same save
  version and install the matching core/chapter needed by its story position;
  this tool does not convert future versions down to older engines.
- Current FSV9 records contain4684 bytes, including the party, EXP/JP, inventory,
  guests, eggs/calendar/RNG, story progress, propositions and separate poach stock.
- Reentry uses the game's existing campaign checkpoints. Raw PC `.state` dumps
  (F2/F3 or `--save`/`--load`) retain platform-specific padding/word order and
  exclude other battle buffers. They are rejected; arbitrary mid-turn resume
  across PC/TI would need a separate portable full-battle snapshot format.
- No ABI, game save layout, engine binary, shipping package, RAM allocation or
  visual changes are required for this exchange.

## Bounded verification

```sh
rtk proxy make -C games/fft/campaign save-transfer-check
rtk proxy make -C games/fft/campaign save-transfer-check SAVE_TRANSFER_FLAGS=--native
```

The first command covers codec boundaries/all slots and the actual campaign
reader/writer on PC and ASan/UBSan. The second adds one small68000 `ti-cycles`
carrier using real TI runtime file I/O: imported PC slot1 is read, an existing
archived slot2 is atomically replaced and exported, then extracted and loaded
again on PC under sanitizers. Every4684 payload byte matches. An FSV8 migration
control also produces the same result on PC and68000. No screenshots or TiEmu
are needed; this is not a new hardware RAM or full-chapter certification.

Default seed is the accepted end-to-end pilot's
`x/pilot-e2e/final/pc/new-game-to-world-save.sav`; supply another real FSV9 seed
with `SAVE_TRANSFER_FLAGS='--seed /path/checkpoint.sav'`. Output defaults to
`x/save-transfer/latest`; override `SAVE_TRANSFER_OUTPUT` to keep accepted proofs
intact. Delivery evidence is `x/save-transfer/final/`, including `results.json`,
PC/sanitizer/native logs, the native `ffsave2.89y` and its decoded `roundtrip.sav`.
