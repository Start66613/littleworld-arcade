#!/usr/bin/env python3
"""Convert every .bmp in a directory to .png (needs Pillow)."""
import os
import sys

try:
    from PIL import Image
except ImportError:
    print("Pillow is not installed; skipping conversion")
    sys.exit(1)


def main():
    directory = sys.argv[1] if len(sys.argv) > 1 else "reports/screenshots"
    converted = 0
    for name in sorted(os.listdir(directory)):
        if not name.lower().endswith(".bmp"):
            continue
        source = os.path.join(directory, name)
        target = os.path.join(directory, os.path.splitext(name)[0] + ".png")
        Image.open(source).save(target)
        converted += 1
    print("converted %d screenshot(s) to PNG" % converted)
    return 0


if __name__ == "__main__":
    sys.exit(main())
