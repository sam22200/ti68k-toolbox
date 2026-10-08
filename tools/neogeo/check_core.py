#!/usr/bin/env python3
"""Compiled-core checks without firmware: ABI/flags and invalid-set rejection."""
import ctypes as C
from pathlib import Path
import tempfile
import zipfile

from neogeorun import DEFAULT_CORE, NeoGeo


def main():
    if not DEFAULT_CORE.is_file():
        raise SystemExit('missing local core; run make -C tools/neogeo core')
    lib = C.CDLL(str(DEFAULT_CORE))
    lib.retro_ti_neogeo_info.argtypes = [C.c_uint]
    lib.retro_ti_neogeo_info.restype = C.c_int32
    lib.retro_ti_neogeo_region.argtypes = [C.c_uint]
    lib.retro_ti_neogeo_region.restype = C.c_void_p
    lib.retro_ti_neogeo_size.argtypes = [C.c_uint]
    lib.retro_ti_neogeo_size.restype = C.c_uint32
    assert lib.retro_api_version() == 1
    assert [lib.retro_ti_neogeo_info(i) for i in (100, 101, 102, 103, 104)] == [1, 1, 0, 0, 1]
    assert lib.retro_ti_neogeo_region(0) is None and lib.retro_ti_neogeo_size(0) == 0
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / 'neogeo.zip'
        # Deliberately invalid archive: this tests error handling, not BIOS emulation.
        with zipfile.ZipFile(path, 'w') as archive:
            archive.writestr('invalid-bios.bin', b'invalid test data, no firmware')
        try:
            neo = NeoGeo(bios=path)
        except ValueError as error:
            assert 'driver did not initialize' in str(error), str(error)
        else:
            neo.close()
            raise AssertionError('invalid BIOS accepted')
    assert lib.retro_ti_neogeo_region(0) is None
    print('Compiled FBNeo: ABI1/word-order/disabled hacks verified; invalid BIOS rejected '
          'and cleaned up; no game boot claimed')


if __name__ == '__main__':
    main()
