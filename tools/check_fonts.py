#!/usr/bin/env python3
"""Verify that every non-ASCII character used by the UI exists in the generated fonts.

The UI font is a subset rasterised from the source files, so a missing glyph shows
up as a blank box on the device. This check makes that a build-time error instead.

Usage: python tools/check_fonts.py [--fonts arcade_font_22 arcade_font_28 ...]
Exit code 0 = every character is covered.
"""
import argparse
import glob
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FIRMWARE = os.path.join(ROOT, "firmware")
FONT_DIR = os.path.join(FIRMWARE, "components", "arcade_ui", "fonts")


def used_characters():
    chars = {}
    for pattern in ("**/*.c", "**/*.h"):
        for path in glob.glob(os.path.join(FIRMWARE, pattern), recursive=True):
            normalised = path.replace("\\", "/")
            if "/build/" in normalised or "/managed_components/" in normalised or "/fonts/" in normalised:
                continue
            try:
                text = open(path, encoding="utf-8").read()
            except UnicodeDecodeError as error:
                print("FAIL  %s is not valid UTF-8: %s" % (path, error))
                continue
            for char in text:
                code = ord(char)
                if 0x20 <= code < 0x7F:
                    continue
                if code < 0x20:
                    continue
                chars.setdefault(char, set()).add(os.path.relpath(path, ROOT))
    return chars


def font_glyphs(font_name):
    path = os.path.join(FONT_DIR, font_name + ".c")
    if not os.path.exists(path):
        return None
    text = open(path, encoding="utf-8", errors="replace").read()
    found = set()
    for match in re.finditer(r"/\* U\+([0-9A-Fa-f]{4,6}) ", text):
        found.add(int(match.group(1), 16))
    return found


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--fonts", nargs="*", default=["arcade_font_22", "arcade_font_28"])
    args = parser.parse_args()

    chars = used_characters()
    print("non-ASCII characters used by the UI: %d" % len(chars))

    failures = 0
    for font_name in args.fonts:
        glyphs = font_glyphs(font_name)
        if glyphs is None:
            print("FAIL  font not found: %s.c" % font_name)
            failures += 1
            continue
        missing = []
        for char, files in sorted(chars.items()):
            if ord(char) not in glyphs:
                missing.append((char, hex(ord(char)), sorted(files)[0]))
        # ASCII coverage is required too (digits, latin labels).
        for code in range(0x20, 0x7F):
            if code not in glyphs:
                missing.append((chr(code), hex(code), "ASCII range"))
        if missing:
            failures += 1
            print("FAIL  %s: %d missing glyph(s)" % (font_name, len(missing)))
            for char, code, where in missing[:20]:
                print("        %s %s  (used in %s)" % (code, char, where))
        else:
            print("PASS  %s: %d glyphs, all UI characters covered" % (font_name, len(glyphs)))

    print("\n%d font(s), %d failure(s)" % (len(args.fonts), failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
