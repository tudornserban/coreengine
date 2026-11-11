#!/usr/bin/env python3
"""CTest helper: run coreengine_sdl2_viewer with --preset collision and sample one pixel."""
import struct
import subprocess
import sys
import zlib


def read_png_rgb(path):
    data = open(path, "rb").read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise AssertionError("not a PNG")
    width, height = struct.unpack(">II", data[16:24])
    index = 8
    idat = b""
    while index < len(data):
        length = struct.unpack(">I", data[index : index + 4])[0]
        chunk_type = data[index + 4 : index + 8]
        index += 8
        chunk = data[index : index + length]
        index += length + 4
        if chunk_type == b"IDAT":
            idat += chunk
        if chunk_type == b"IEND":
            break
    raw = zlib.decompress(idat)
    offset = 0
    rows = []
    for _ in range(height):
        offset += 1
        rows.append(raw[offset : offset + 3 * width])
        offset += 3 * width
    return width, height, rows


def pixel_rgb(rows, _width, x, y):
    index = x * 3
    row = rows[y]
    return row[index], row[index + 1], row[index + 2]


def main():
    if len(sys.argv) != 3:
        print("usage: check_collision_preset_cli.py VIEWER_EXECUTABLE OUTPUT_PNG", file=sys.stderr)
        sys.exit(1)
    viewer, out_path = sys.argv[1], sys.argv[2]
    result = subprocess.run(
        [
            viewer,
            "--no-window",
            "--preset",
            "collision",
            "--frames",
            "1",
            "--screenshot",
            out_path,
        ],
        check=False,
    )
    if result.returncode != 0:
        print(f"viewer exit {result.returncode}", file=sys.stderr)
        sys.exit(result.returncode)
    _w, _h, rows = read_png_rgb(out_path)
    # Team 1 at world (380,300) -> screen ~(618.67,360) -> pixel (619,360) RGB (70,140,230)
    r, g, b = pixel_rgb(rows, _w, 619, 360)
    if (r, g, b) != (70, 140, 230):
        print(f"pixel (619,360) got {(r,g,b)} expected (70,140,230)", file=sys.stderr)
        sys.exit(1)
    print("OK collision preset CLI smoke")


if __name__ == "__main__":
    main()
