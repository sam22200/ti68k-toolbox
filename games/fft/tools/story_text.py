"""Shared local-disc event decoder for the bounded native story adapters."""
import struct
import management_research as text

def message(raw,event,mid):
    text.PUNCT.update({2064:'"',2073:'?',64:"'",234:'!'})
    b=raw[event*8192:(event+1)*8192];at=struct.unpack_from('<I',b)[0]
    starts=[at]+[i+1 for i in range(at,8192) if b[i]>=254]
    tokens=[]
    for c in text.expanded(b,starts[mid]):
        tokens.extend([62] if c==234 else [62,63] if c==64 else [217,193] if c==147 else [c])
    lines,unknown,dynamic=text.decode(tokens)
    if unknown:raise ValueError((event,mid,unknown))
    name=lines.pop(0) if mid or event!=2 else 'Ovelia'
    body=' '.join(s.strip() for s in lines if s!='|').strip()
    # An unnamed one-line shout has no speaker field (Academy: "Everyone, fall in!").
    if not body and name.strip():body=name.strip();name=''
    for a,b in (("What s","What's"),("He s","He's"),("There s","There's"),("there s","there's"),
                ("We can t","We can't"),("Don t","Don't"),("You re","You're"),("Goltana s","Goltana's")):
        body=body.replace(a,b)
    return name.strip(),body
