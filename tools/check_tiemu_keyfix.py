#!/usr/bin/env python3
"""Check GTK event ordering on a private X server; leave the user's TiEmu untouched."""
import os
from pathlib import Path
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]


def main():
    subprocess.run(['docker', 'build', '--target', 'keyfix', '-q', '-t', 'tiemu-keyfix-test',
                    '-f', str(ROOT / 'tools/Dockerfile.tiemu'), str(ROOT / 'tools')], check=True)
    number = next(n for n in range(99, 199) if not Path('/tmp/.X11-unix/X%d' % n).exists())
    with tempfile.TemporaryDirectory(prefix='tiemu-keyfix-') as work:
        auth = Path(work) / 'xauth'; auth.touch()
        display = ':%d' % number
        server = subprocess.Popen(['Xvfb', display, '-screen', '0', '640x480x24', '-ac'],
                                  stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        try:
            for _ in range(50):
                if Path('/tmp/.X11-unix/X%d' % number).exists(): break
                if server.poll() is not None: raise RuntimeError('Private X server exited')
                time.sleep(.05)
            subprocess.run(['docker', 'run', '--rm', '--network', 'host',
                            '-u', '%d:%d' % (os.getuid(), os.getgid()),
                            '-e', 'DISPLAY='+display, '-e', 'XAUTHORITY='+str(auth),
                            '-v', str(ROOT)+':'+str(ROOT)+':ro', '-v', work+':'+work,
                            'tiemu-keyfix-test', 'sh', '-c',
                            'gcc -O2 -Wall -Wextra -Wno-unused-parameter -o "$1/test" "$2/tools/tiemu-keyfix-test.c" '
                            '$(pkg-config --cflags --libs gtk+-2.0) -lX11 -ldl && "$1/test"',
                            'sh', work, str(ROOT)], check=True)
        finally:
            server.terminate(); server.wait(timeout=5)


if __name__ == '__main__': main()
