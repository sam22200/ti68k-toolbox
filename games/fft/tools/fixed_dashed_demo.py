#!/usr/bin/env python3
"""Compare previous full cutaway with near-camera removal / dotted architectural edges."""
from fixed_demo import HERE
from fixed_camera_demo import main


if __name__ == '__main__':
    main(HERE/'x/fixed-dashed', HERE/'x/fixed-elevated')
