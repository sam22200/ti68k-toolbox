# Corrections to the vendored GCC4TI sources

`tools/gcc4ti/` (the checkout) and `tools/gcc4ti-bin/` (the installed toolchain)
are not in git, so anything we change in them lives here instead.
`tools/build-gcc4ti.sh` applies every patch in this directory to
`tools/gcc4ti/trunk` before building, and skips one that is already applied.

To rebuild a single tool without the whole toolchain:

```sh
cd tools/gcc4ti/trunk/tigcc/tools    && mkdir -p bin && make ttpack ttunpack
cd tools/gcc4ti/trunk/tigcc/ld-tigcc && make
cp tools/gcc4ti/trunk/tigcc/tools/bin/ttpack tools/gcc4ti-bin/bin/
cp tools/gcc4ti/trunk/tigcc/ld-tigcc/ld-tigcc tools/gcc4ti-bin/bin/
```

## 0001 - ttpack: the 16-bit size field is the only TI limit

`-pack` refused any program over 64,936 bytes. The number is
`0xffff - 0x258 + 1`, and `0x258` is pucrunch's **C64 load address**: the check
read `startAddr + inlen - 1 > 0xffff`.

On the TI output path (`(type & FIXF_MACHMASK) == 0`) that address is used
nowhere. `SavePack` writes the sixteen-byte `PackedHeader` - original size,
magic, compressed size, escape, gamma, extra LZ bits, RLE - and no address;
`PackLz77` leaves `endAddr`/`memEnd` alone on that path; and the decompressor
bounds its output with the header's **16-bit** original-size field while every LZ
reference it follows is relative to the output pointer. So the real limit is
65,535 and the other 600 bytes were a C64 artefact. Other machine types keep
their own bound.

Checked before shipping it:

- the packed output is **byte for byte identical** below the old limit (FFT's
  battle core, 45,414 bytes, patched and unpatched packer);
- a real 65,059-byte image **round-trips exactly** through `ttpack` then
  `ttunpack`.

What it bought: `games/fft`'s chapter battle program, which was at the ceiling
with 335 bytes free, can carry both the status pictograms and the transformation
creatures. See `games/fft/campaign/RESIDENT_BUDGET.md`.
