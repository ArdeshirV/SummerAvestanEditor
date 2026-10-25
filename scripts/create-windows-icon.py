#!/usr/bin/env python3
"""Create Icon.ico from Icon.png for Windows executable resources."""
import ctypes
import ctypes.util
import struct
import sys
from pathlib import Path

# libpng's simplified image API emits normalized 8-bit RGBA pixels. Keep the
# ctypes structure limited to its public header fields; extra storage is opaque.
class PngImage(ctypes.Structure):
    _fields_ = [
        ("opaque", ctypes.c_void_p),
        ("version", ctypes.c_uint),
        ("width", ctypes.c_uint),
        ("height", ctypes.c_uint),
        ("format", ctypes.c_uint),
        ("flags", ctypes.c_uint),
        ("colormap_entries", ctypes.c_uint),
        ("warning_or_error", ctypes.c_uint),
        ("message", ctypes.c_char * 64),
        ("opaque_storage", ctypes.c_ubyte * 256),
    ]


def png_rgba(path: Path):
    library = ctypes.util.find_library("png16") or ctypes.util.find_library("png")
    if not library:
        raise RuntimeError("libpng is required to generate Icon.ico")
    png = ctypes.CDLL(library)
    begin = png.png_image_begin_read_from_file
    begin.argtypes = [ctypes.POINTER(PngImage), ctypes.c_char_p]
    begin.restype = ctypes.c_int
    finish = png.png_image_finish_read
    finish.argtypes = [ctypes.POINTER(PngImage), ctypes.c_void_p, ctypes.c_void_p,
                       ctypes.c_int, ctypes.c_void_p]
    finish.restype = ctypes.c_int
    free = png.png_image_free
    free.argtypes = [ctypes.POINTER(PngImage)]

    image = PngImage()
    image.version = 1
    if not begin(ctypes.byref(image), str(path).encode()):
        raise RuntimeError("libpng could not read Icon.png")
    image.format = 0x03  # PNG_FORMAT_RGBA
    pixels = (ctypes.c_ubyte * (image.width * image.height * 4))()
    if not finish(ctypes.byref(image), None, pixels, 0, None):
        message = bytes(image.message).split(b"\0", 1)[0].decode(errors="replace")
        free(ctypes.byref(image))
        raise RuntimeError(f"libpng could not decode Icon.png: {message}")
    w, h = image.width, image.height
    free(ctypes.byref(image))
    return w, h, bytes(pixels)


def resize_rgba(src, sw, sh, dw, dh):
    # Bilinear resampling with premultiplied-alpha interpolation to avoid
    # dark fringes around transparent icon edges.
    output = bytearray(dw * dh * 4)
    for y in range(dh):
        fy = (y + .5) * sh / dh - .5
        y0 = max(0, min(sh - 1, int(fy)))
        y1 = min(sh - 1, y0 + 1)
        wy = max(0.0, min(1.0, fy - int(fy)))
        for x in range(dw):
            fx = (x + .5) * sw / dw - .5
            x0 = max(0, min(sw - 1, int(fx)))
            x1 = min(sw - 1, x0 + 1)
            wx = max(0.0, min(1.0, fx - int(fx)))
            samples = []
            for xx, yy, weight in ((x0,y0,(1-wx)*(1-wy)), (x1,y0,wx*(1-wy)),
                                   (x0,y1,(1-wx)*wy), (x1,y1,wx*wy)):
                i = (yy * sw + xx) * 4
                a = src[i+3] / 255.0
                samples.append((src[i] * a, src[i+1] * a, src[i+2] * a, src[i+3], weight))
            alpha = sum(s[3] * s[4] for s in samples)
            oi = (y * dw + x) * 4
            if alpha > 0:
                for c in range(3):
                    premul = sum(s[c] * s[4] for s in samples)
                    output[oi+c] = max(0, min(255, round(premul * 255 / alpha)))
            output[oi+3] = max(0, min(255, round(alpha)))
    return bytes(output)


def dib_image(rgba, size):
    # ICO DIB uses bottom-up BGRA pixels plus a 1-bit AND mask.
    xor = bytearray()
    mask = bytearray()
    stride = (size * 4 + 3) & ~3
    mask_stride = ((size + 31) // 32) * 4
    for y in range(size - 1, -1, -1):
        row = bytearray()
        mask_row = bytearray(mask_stride)
        for x in range(size):
            i = (y * size + x) * 4
            r, g, b, a = rgba[i:i+4]
            row.extend((b, g, r, a))
            if a < 128:
                mask_row[x // 8] |= 0x80 >> (x % 8)
        xor.extend(row)
        xor.extend(b'\0' * (stride - len(row)))
        mask.extend(mask_row)
    header = struct.pack('<IIIHHIIIIII', 40, size, size * 2, 1, 32, 0,
                         len(xor) + len(mask), 0, 0, 0, 0)
    return header + xor + mask


def main():
    root = Path(__file__).resolve().parent.parent
    source, destination = root / 'Icon.png', root / 'Icon.ico'
    try:
        sw, sh, pixels = png_rgba(source)
        sizes = (256, 128, 64, 48, 32, 16)
        images = [(size, dib_image(resize_rgba(pixels, sw, sh, size, size), size))
                  for size in sizes]
        offset = 6 + 16 * len(images)
        directory = bytearray(struct.pack('<HHH', 0, 1, len(images)))
        payload = bytearray()
        for size, data in images:
            directory.extend(struct.pack('<BBBBHHII', size & 255, size & 255, 0, 0,
                                         1, 32, len(data), offset))
            payload.extend(data)
            offset += len(data)
        destination.write_bytes(directory + payload)
        print(f'Created {destination} ({len(images)} sizes)')
    except Exception as exc:
        print(f'Could not create Icon.ico: {exc}', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
