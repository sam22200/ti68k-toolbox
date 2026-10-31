"""Publish review images only when their encoded content changes.

Native preview programs render into a temporary directory; unchanged captures in
the gallery are never rewritten, so their modification times remain truthful.
"""
from io import BytesIO
from pathlib import Path


def write_changed(path, data):
    path=Path(path)
    if path.exists() and path.read_bytes()==data:
        return False
    path.parent.mkdir(parents=True,exist_ok=True)
    path.write_bytes(data)
    return True


def save_image(image,path,**options):
    path=Path(path);buf=BytesIO()
    image.save(buf,format='GIF' if path.suffix.lower()=='.gif' else 'PNG',**options)
    return write_changed(path,buf.getvalue())


def publish_images(source,destination):
    source,destination=Path(source),Path(destination)
    for path in source.rglob('*'):
        if path.suffix.lower() in ('.png','.gif') and path.is_file():
            write_changed(destination/path.relative_to(source),path.read_bytes())
