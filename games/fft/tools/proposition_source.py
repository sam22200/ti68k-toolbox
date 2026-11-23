"""Bounded original Propositions sample for service design; no shipping/save writes."""
import hashlib
import struct
import bar_assets
import extract
import item_help
import management_data as md
import management_research as mr


def sample(cue,index=30):
    if not 0<=index<96:raise ValueError('Proposition index')
    disc,files=extract.disc_files(cue)
    names=('WORLD/WLDCORE.BIN','EVENT/WORLD.LZW','WORLD/WLDMES.BIN')
    raw={name:disc.read(*files[name]) for name in names};core,world,messages=(raw[n] for n in names)
    base=struct.unpack_from('<I',core,0x8009d340-0x80067000)[0]-0x80067000
    offsets=struct.unpack_from('<15I',core,base)
    row=core[base+offsets[0]+23*index:base+offsets[0]+23*(index+1)]
    if len(row)!=23 or row[2]!=index+1:raise ValueError('Source proposition row')
    fees=struct.unpack_from('<10H',core,base+offsets[1])
    sections=struct.unpack_from('<32I',world)
    titles=item_help.packed_entries(world,128+sections[26],128+sections[27])
    title=md.decode(bytes(mr.expanded(world,titles[index]))[:-1])
    # The original loader uses22-sector message blocks;1 is Propositions.
    blocks=struct.unpack_from('<3I',core,0x8009e470-0x80067000)
    at=2048*blocks[1]
    for _ in range(index):at=messages.index(254,at)+1
    encoded=messages[at:messages.index(254,at)+1]
    # Remove only the retail Circle-button footer; native controls replace it.
    button=encoded.index(bytes((0xd9,185)))
    footer=encoded.rfind(bytes((0xd9,190)),0,button)
    if footer<0:raise ValueError('Source button footer boundary')
    description,effects=bar_assets.prose(encoded[:footer]+bytes((254,)))
    if effects:raise ValueError('Review sample has unhandled story effects')
    return dict(id=index,title=title,fee=fees[row[4]]+fees[row[5]],
                min_days=row[6],max_days=row[7],location=row[16]-1,
                required_stock=row[21],condition_flags=row[17],row=list(row),
                description=description,description_offset=at,footer_removed_at=footer,
                source_sha256={n:hashlib.sha256(b).hexdigest() for n,b in raw.items()},
                scope='Original row30, Chapter II Gariland design fixture; not made available in Chapter I.')
