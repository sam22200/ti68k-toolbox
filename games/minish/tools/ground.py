"""Palette-aware earth reduction; original soil/grass motifs stay intact."""
import numpy as np
from art import grayscale


def ground_gray(rgb):
    level=grayscale(rgb)
    # Brown earth and green grass have similar luminance in the ROM. Separate
    # those hues on the LCD; keep the original darker soil flecks and edges.
    earth=(rgb[:,:,0]>rgb[:,:,1])&(rgb[:,:,1]>rgb[:,:,2])
    level[earth]=np.where(rgb[:,:,0][earth]>175,0,2)
    return level
