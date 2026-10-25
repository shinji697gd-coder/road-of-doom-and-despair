"""Prepare a Geode logo from an authentic online Nullscapes gameplay capture.

Source: Geometry Dash Fan Wiki, Nullscapes page (original Kiba level).
Only resize/crop; do not generate or alter imagery.
"""
from io import BytesIO
from pathlib import Path
import urllib.request

from PIL import Image, ImageOps

SOURCES = [
    "https://static.wikia.nocookie.net/geometry-dash-fan/images/8/8a/Nullscapes.png/revision/latest/scale-to-width-down/1200?cb=20240906150718",
    "https://static.wikia.nocookie.net/geometry-dash-fan/images/8/8a/Nullscapes.png",
]
last_error = None
for source in SOURCES:
    try:
        req = urllib.request.Request(source, headers={
            "User-Agent": "Mozilla/5.0",
            "Accept": "image/avif,image/webp,image/apng,image/*,*/*;q=0.8",
        })
        with urllib.request.urlopen(req, timeout=20) as response:
            raw = response.read()
        im = Image.open(BytesIO(raw))
        print(f"Downloaded genuine screenshot: format={im.format}, size={im.size}, bytes={len(raw)}")
        im.load()
        if im.width < 128 or im.height < 128:
            raise RuntimeError("Image too small to use as icon")
        # Image resizing is not AI generation. Keep the exact game image intact.
        logo = ImageOps.fit(im.convert("RGBA"), (512, 512), method=Image.Resampling.LANCZOS)
        logo.save("logo.png", format="PNG", optimize=True)
        saved = Image.open("logo.png")
        assert saved.size == (512, 512) and saved.format == "PNG"
        print("Wrote real gameplay image as 512x512 PNG (logo.png)")
        break
    except Exception as exc:
        last_error = exc
        print(f"Source unavailable: {exc}")
else:
    raise RuntimeError("Could not download a genuine Nullscapes image") from last_error
