"""US Chapter I rumor transitions, read from the original event instruction stream.

Native flags are declared by chapter authoring, one-based like FCD/FCX/manifest
flags. No story event or progression is inferred from opening a Bar reader.
The pilot binds only event00C to flag2 (Gariland victory); later bindings are
required when the corresponding scenes/encounters are authored.
"""
import hashlib
import struct

EVENTS=(0x00c,0x02d,0x03a,0x075)
# Observed source intervals; checked against decoded instructions, not byte searches.
INTERVALS=((0x00c,0x03a),(0x00c,0x02d),(0x00c,0x075),
           (0x02d,0x075),(0x03a,0x075),(0x03a,0x075))


def bindings(rows,complete=False):
    result={};used=set()
    for row in rows:
        if set(row)!={'event','flag'}:raise ValueError('Rumor event needs event/flag only')
        event,flag=row['event'],row['flag']
        if type(event)!=int or type(flag)!=int or event not in EVENTS or not 1<=flag<=255:
            raise ValueError('Invalid source rumor event/native flag binding')
        if event in result or flag in used:raise ValueError('Duplicate rumor event/flag binding')
        result[event]=flag;used.add(flag)
    if complete and set(result)!=set(EVENTS):raise ValueError('Incomplete Chapter I rumor transitions')
    return result


def gates(rows,complete=False):
    bound=bindings(rows,complete);out=[]
    for identity,(first,last) in enumerate(INTERVALS):
        if first in bound:
            out.append((identity,bound[first]-1,bound[last]-1 if last in bound else 255))
    return out


def read_source(events,sizes):
    """Decode four bounded event blocks; require their final straight-line assignments.

    This is source inspection, not a retail replay of the complete scenes. The
    assignments occur in the unconditional trailing sequence after the dialogue.
    New source control flow is rejected rather than treated as a linear script.
    """
    transitions=[]
    for event in EVENTS:
        block=events[event*8192:(event+1)*8192]
        if len(block)!=8192:raise ValueError('Missing source event block')
        end=struct.unpack_from('<I',block)[0]
        if not 4<end<8192:raise ValueError('Invalid source instruction boundary')
        instructions=[];at=4
        while at<end:
            op=block[at];length=1+sizes[op]
            if at+length>end:raise ValueError('Instruction exceeds source event boundary')
            instructions.append((at,op,block[at+1:at+length]));at+=length
        if instructions[-1][1]!=0xdb:raise ValueError('Source event must end normally')
        assignments=[];tail=None;pending=None
        for offset,op,data in instructions:
            if op==0xbe:
                if len(data)!=2:raise ValueError('ZeroVariable operand size')
                pending=struct.unpack('<H',data)[0]
                if pending==0x6e:tail=offset
            elif op==0xb0 and pending is not None:
                if len(data)!=4:raise ValueError('AddImmediate operand size')
                variable,value=struct.unpack('<2H',data)
                if variable!=pending:raise ValueError('Non-paired source variable assignment')
                if variable==0x6e or 0x2d1<=variable<0x311:
                    if tail is None:raise ValueError('Rumor write outside final progress sequence')
                    assignments.append(dict(offset=offset,variable=variable,value=value))
                pending=None
            else:
                pending=None
        if tail is None or any(0xd0<=op<=0xd6 for offset,op,_ in instructions if offset>=tail):
            raise ValueError('Conditional rumor transition needs an explicit source consumer')
        relevant=[r for r in assignments if r['offset']>=tail]
        progress=[r['value'] for r in relevant if r['variable']==0x6e]
        writes={r['variable']-0x2d1:r['value'] for r in relevant if r['variable']!=0x6e}
        if len(progress)!=1 or any(v not in (0,1) for v in writes.values()):
            raise ValueError('Unexpected source rumor state')
        transitions.append(dict(event=event,progress=progress[0],tail=tail,
                                writes=sorted([key,value] for key,value in writes.items()),assignments=relevant,
                                block_sha256=hashlib.sha256(block).hexdigest()))
    # Derive source lifetimes independently of the hand-written authoring constants.
    first={};last={};live=set()
    for row in transitions:
        for identity,value in row['writes']:
            if identity>=6:continue  # first Chapter II transition also enables6..8
            if value:
                if identity in live or identity in first:raise ValueError('Non-interval rumor lifetime')
                first[identity]=row['event'];live.add(identity)
            elif identity in live:last[identity]=row['event'];live.remove(identity)
    if live or tuple((first[i],last[i]) for i in range(6))!=INTERVALS:
        raise ValueError('Source Chapter I rumor intervals changed')
    return transitions


def validate_bank(raw,rows):
    expected=gates(rows)
    actual=[(raw[at],raw[at+1],raw[at+2]) for at in range(8,8+14*raw[6],14)]
    if actual!=expected:raise ValueError('Rumor bank does not match declared source-event bindings')
