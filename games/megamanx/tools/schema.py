"""Shared explicit-word trace width, independent of the PC/TI struct ABI."""
import re
from reference import GAME
WORDS=int(re.search(r'^#define MMX_WORDS (\d+)',(GAME/'mmx.h').read_text(),re.M)[1])
