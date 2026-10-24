#!/usr/bin/env python3
"""Offline sanity check of a full ESP32-P4 flash dump produced by backup_flash.ps1.

It does not need the board. It verifies that the dump looks like a real ESP-IDF
flash image: bootloader header at 0x2000, partition table (0x8000 or 0x9000),
and a bootable application at the factory offset.

Usage: python tools/check_backup.py <dump.bin> [--expect-size 33554432]
"""
import argparse
import hashlib
import json
import struct
import sys

CHIP_NAMES = {0x0000: "esp32", 0x0009: "esp32s3", 0x000D: "esp32c6", 0x0012: "esp32p4"}
PART_TYPES = {0x00: "app", 0x01: "data"}
APP_SUBTYPES = {0x00: "factory", 0x10: "ota_0", 0x11: "ota_1"}
DATA_SUBTYPES = {0x00: "ota", 0x01: "phy", 0x02: "nvs", 0x03: "coredump", 0x04: "nvs_keys",
                 0x05: "efuse", 0x06: "undefined", 0x80: "esphttpd", 0x81: "fat", 0x82: "spiffs"}


def image_header(data, offset):
    if offset + 24 > len(data) or data[offset] != 0xE9:
        return None
    chip_id = struct.unpack_from("<H", data, offset + 12)[0]
    min_rev = struct.unpack_from("<H", data, offset + 15)[0]
    max_rev = struct.unpack_from("<H", data, offset + 17)[0]
    segments = data[offset + 1]
    return {
        "offset": offset,
        "chip": CHIP_NAMES.get(chip_id, "unknown(0x%04X)" % chip_id),
        "segments": segments,
        "min_rev_full": min_rev,
        "max_rev_full": max_rev,
    }


def partition_table(data, offset):
    if offset + 32 > len(data) or data[offset] != 0xAA or data[offset + 1] != 0x50:
        return None
    entries = []
    for pos in range(offset, min(len(data) - 31, offset + 0xC00), 32):
        magic, ptype, subtype, addr, size, label, flags = struct.unpack_from("<HBBII16sI", data, pos)
        if magic != 0x50AA:
            break
        entries.append({
            "name": label.split(b"\x00")[0].decode("ascii", "replace"),
            "type": PART_TYPES.get(ptype, "0x%02X" % ptype),
            "subtype": (APP_SUBTYPES if ptype == 0 else DATA_SUBTYPES).get(subtype, "0x%02X" % subtype),
            "offset": hex(addr),
            "size": hex(size),
        })
    return entries


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("dump")
    parser.add_argument("--expect-size", type=int, default=33554432)
    parser.add_argument("--json")
    args = parser.parse_args()

    data = open(args.dump, "rb").read()
    report = {
        "file": args.dump,
        "size": len(data),
        "sha256": hashlib.sha256(data).hexdigest(),
        "size_ok": len(data) == args.expect_size,
        "bootloader": image_header(data, 0x2000),
        "partition_table": None,
        "table_offset": None,
        "app": None,
    }
    print("file    : %s" % args.dump)
    print("size    : %d bytes (expected %d) %s" % (len(data), args.expect_size,
                                                   "OK" if report["size_ok"] else "MISMATCH"))
    print("sha256  : %s" % report["sha256"])

    for offset in (0x9000, 0x8000):
        entries = partition_table(data, offset)
        if entries:
            report["partition_table"] = entries
            report["table_offset"] = offset
            break

    print("")
    if report["bootloader"] is None:
        print("bootloader: NOT FOUND at 0x2000")
    else:
        boot = report["bootloader"]
        print("bootloader: chip=%s segments=%d revision v%d.%d..v%d.%d" % (
            boot["chip"], boot["segments"], boot["min_rev_full"] // 100, boot["min_rev_full"] % 100,
            boot["max_rev_full"] // 100, boot["max_rev_full"] % 100))

    if report["partition_table"] is None:
        print("partition table: NOT FOUND at 0x8000 or 0x9000")
    else:
        print("partition table at 0x%X:" % report["table_offset"])
        for entry in report["partition_table"]:
            print("  %-10s %-5s %-8s offset=%s size=%s" % (
                entry["name"], entry["type"], entry["subtype"], entry["offset"], entry["size"]))

    # Locate the factory app and confirm its header.
    factory = None
    if report["partition_table"]:
        for entry in report["partition_table"]:
            if entry["type"] == "app":
                factory = entry
                break
    if factory:
        offset = int(factory["offset"], 16)
        app = image_header(data, offset)
        report["app"] = app
        if app:
            print("app        : %s at %s, chip=%s revision v%d.%d..v%d.%d" % (
                factory["name"], factory["offset"], app["chip"],
                app["min_rev_full"] // 100, app["min_rev_full"] % 100,
                app["max_rev_full"] // 100, app["max_rev_full"] % 100))
        else:
            print("app        : no image header at %s" % factory["offset"])

    if args.json:
        with open(args.json, "w", encoding="utf-8") as handle:
            json.dump(report, handle, ensure_ascii=False, indent=2)
        print("\nreport: %s" % args.json)

    ok = report["size_ok"] and report["bootloader"] is not None and report["partition_table"] is not None
    print("\nRESULT: %s" % ("dump looks coherent" if ok else "dump is incomplete or inconsistent"))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
