#!/usr/bin/env python3
"""Produce review-only choice/roster-full pictures with the existing renderer."""
from pathlib import Path
import tempfile
from PIL import Image, ImageDraw
from check_flow import HERE, ROOT, data_links, run


def main():
    out=HERE/'x/story-choices/review';out.mkdir(parents=True,exist_ok=True)
    (out/'build.log').write_text(run(['cc','-std=gnu99','-O1','-g','-w','-o',out/'preview',
        HERE/'review_story_choices.c',ROOT/'runtime/core/rt_core.c',ROOT/'runtime/platform-sw/rt_sw.c']))
    with tempfile.TemporaryDirectory(prefix='fft-story-choice-review-') as folder:
        data=Path(folder);data_links(data,HERE/'x/guests/shipping')
        log=run([out/'preview',out],data);(out/'review.log').write_text(log);print(log.strip())
    canvas=Image.new('RGB',(1488,328),'white');draw=ImageDraw.Draw(canvas)
    for i,(name,label) in enumerate([('before','Existing dialogue'),('choice','Recruitment choice'),('full','Party full: choice stays open')]):
        draw.text((12+496*i,7),label,fill='black')
        canvas.paste(Image.open(out/(name+'.png')).resize((480,300),Image.Resampling.NEAREST),(8+496*i,24))
    canvas.save(out/'review.png');print(out/'review.png')


if __name__=='__main__':main()
