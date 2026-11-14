"""Bounded retail SEQ timelines, with GPU/audio output isolated.

Execute the original unpacker, script player and coordinate-shift helpers. This
is a body-pose oracle, not a battle simulation: no victims, effects or audio.
"""
import struct

import monster_ko_review as source
from status_state_oracle import bind_function


class Timeline(source.Original):
    def __init__(self, disc):
        import unicorn
        self.extra = {}
        for key, symbol in (
                ('shift_rl', 'battle_unit_shift_right_or_left'),
                ('shift_du', 'battle_unit_shift_down_or_up'),
                ('shift_fb', 'battle_unit_shift_forward_or_backward'),
                ('attack_unpack', 'battle_gfx_decompress_attack_spritesheet'),
                ('read_nibble', 'battle_gfx_read_compressed_nibble'),
                ('sound_move', 'battle_sound_play_movement_sfx'),
                ('sound_hit', 'battle_sound_select_weapon_hit')):
            self.extra[key] = bind_function(key, symbol, 'battle', disc)
        super().__init__(disc)
        self.hashes.update(self.extra)
        self.calls = []
        self.guard = False
        self.allowed = [self.s.FUNCTIONS[k][:2] for k in
                        ('run_seq', 'shift_rl', 'shift_du', 'shift_fb')]
        isolated = {self.s.FUNCTIONS[k][0]: k for k in
                    ('sound_move', 'sound_hit', 'load_parts')}

        def instruction(uc, address, size, data):
            if not self.guard:
                return
            if address in isolated:
                kind = isolated[address]
                if kind != 'load_parts':
                    self.calls.append(dict(kind=kind, argument=uc.reg_read(
                        self.o.reg.UC_MIPS_REG_A1)))
                    uc.reg_write(self.o.reg.UC_MIPS_REG_PC,
                                 uc.reg_read(self.o.reg.UC_MIPS_REG_RA))
                return
            if not any(start <= address < start+length for start, length in self.allowed):
                raise ValueError(f'unexpected retail dependency at {address:08x}')

        self.o.uc.hook_add(unicorn.UC_HOOK_CODE, instruction)

    def attack_sheet(self, spr):
        """Retail nibble-RLE output; retain exact packed bytes as the oracle."""
        o = self.o
        source_ptr, destination = 0x801c0000, 0x801d3000
        o.uc.mem_write(source_ptr & 0x1fffff, spr[0x9200:])
        o.uc.mem_write(destination & 0x1fffff, bytes([0xa5])*0x6400)
        old_limit = o.instruction_limit
        o.instruction_limit = 3000000
        try:
            o.call('attack_unpack', source_ptr, destination)
        finally:
            o.instruction_limit = old_limit
        return bytes(o.uc.mem_read(destination & 0x1fffff, 0x6400))

    def trace(self, seq, animation, sheet, facing=0, limit=64):
        o = self.o
        source_ptr, table, payload = 0x801c0000, 0x801c4000, 0x801c5000
        unit, state, display = 0x801d0000, 0x801d1000, 0x801d2000
        o.uc.mem_write(source_ptr & 0x1fffff, seq)
        o.write(0x800a1c58, payload)
        o.call('unpack_seq', table, source_ptr)
        record = bytearray(0x440)
        record[6] = sheet
        struct.pack_into('<H', record, 0x70, facing)
        o.uc.mem_write(unit & 0x1fffff, bytes(record))
        record = bytearray(0x30)
        struct.pack_into('<H', record, 4, animation)
        struct.pack_into('<I', record, 0x20, table)
        struct.pack_into('<I', record, 0x2c, display)
        o.uc.mem_write(state & 0x1fffff, bytes(record))
        o.uc.mem_write(display & 0x1fffff, bytes(0x100))
        o.write(0x800960e4, 0x34)  # EVENT: suppress post-attack victim dispatch.
        o.write(0x8009612c, 0)     # No held weapon continuation.
        o.write(0x80098db8, 1)    # Input wait released.
        rows, seen = [], set()
        for _ in range(limit):
            self.frames.clear()
            self.calls.clear()
            self.guard = True
            try:
                o.call('run_seq', unit, state, 1)
            finally:
                self.guard = False
            raw = bytes(o.uc.mem_read(state & 0x1fffff, 0x30))
            anim, pos, frame, wait, restarts = struct.unpack_from('<5H', raw, 4)
            flags = struct.unpack_from('<H', raw, 0x18)[0]
            offset = struct.unpack('<3h', o.uc.mem_read((unit & 0x1fffff)+0x50, 6))
            if len(self.frames) != 1 or self.frames[0][1] != anim:
                raise ValueError('expected one body frame per retail script step')
            loaded = self.frames[0][0]
            # END/PAUSE displays the source default without changing state.frame.
            if wait and loaded != frame:
                raise ValueError('displayed frame disagrees with retail state')
            key = (anim, pos, loaded, wait, flags, offset, raw[0x0e:0x18])
            if restarts or key in seen:
                reason = 'loop'
                break
            seen.add(key)
            rows.append(dict(animation=anim, pos=pos, frame=loaded, wait=wait,
                             flags=flags, offset=list(offset),
                             isolated_calls=list(self.calls)))
            if not wait:
                reason = 'held-or-ended'
                break
        else:
            raise ValueError('retail timeline exceeds bounded study limit')
        if not rows:
            raise ValueError('empty retail timeline')
        return dict(steps=rows, stop=reason)
