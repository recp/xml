"""Crosscheck name spans against libxml2's XML 1.0 Fifth Edition scanner.

The legacy xmlValidateName family and some Expat versions still apply older
name tables. Use xmlParseName, then the NCName/QName colon productions instead.
The system reference is optional; it is never linked into the production code.
"""

import ctypes as C
import ctypes.util
import random
import subprocess
import sys


def reference():
    path = ctypes.util.find_library("xml2")
    if not path:
        print("SKIP: libxml2 name reference unavailable")
        sys.exit(77)
    try:
        lib = C.CDLL(path)
        lib.xmlCreateMemoryParserCtxt.argtypes = [C.c_char_p, C.c_int]
        lib.xmlCreateMemoryParserCtxt.restype = C.c_void_p
        lib.xmlCtxtUseOptions.argtypes = [C.c_void_p, C.c_int]
        lib.xmlCtxtUseOptions.restype = C.c_int
        lib.xmlParseName.argtypes = [C.c_void_p]
        lib.xmlParseName.restype = C.c_void_p
        lib.xmlFreeParserCtxt.argtypes = [C.c_void_p]
        lib.xmlFreeParserCtxt.restype = None
    except (AttributeError, OSError):
        print("SKIP: libxml2 name scanner API unavailable")
        sys.exit(77)

    def name(raw):
        # A delimiter lets the reference terminate normally after the span.
        context = lib.xmlCreateMemoryParserCtxt(raw + b" ", len(raw) + 1)
        if not context:
            raise MemoryError("libxml2 reference context")
        try:
            lib.xmlCtxtUseOptions(context, 32 | 64 | 2048)  # quiet, no network
            parsed = lib.xmlParseName(context)
            return bool(parsed) and C.string_at(parsed) == raw
        finally:
            lib.xmlFreeParserCtxt(context)

    if not name("🙂".encode()) or not name("\u037f".encode()):
        print("SKIP: reference lacks XML 1.0 Fifth Edition name rules")
        sys.exit(77)
    return name


def fixtures():
    rng = random.Random(42916)
    boundaries = [0xB7, 0xC0, 0xD6, 0xD8, 0xF6, 0xF8, 0x2FF, 0x300,
                  0x36F, 0x370, 0x37D, 0x37F, 0x1FFF, 0x200C, 0x200D,
                  0x203F, 0x2040, 0x2070, 0x218F, 0x2C00, 0x2FEF, 0x3001,
                  0xD7FF, 0xF900, 0xFDCF, 0xFDF0, 0xFFFD, 0x10000, 0xEFFFF]
    points = set(range(128))
    points.update(c + delta for c in boundaries for delta in [-1, 0, 1])
    points.update(rng.randrange(0x110000) for _ in range(4096))
    names = {b"", b":", b"::", b"a:", b":a", b"a:b:c", b"a::b", b"a:1",
             b"xml", b"xmlns", b"xml:lang", b"xmlns:p", b"undeclared:a",
             b"a" * 3000, b"p:" + b"a" * 3000}
    for point in points:
        raw = chr(point).encode("utf-8", errors="surrogatepass")
        names.update([raw, b"a" + raw, b"p:" + raw, raw + b":a"])
    for byte in range(256):
        raw = bytes([byte])
        names.update([raw, b"a" + raw, b"p:" + raw])
    for raw in [b"\xc0\x80", b"\xc1\xbf", b"\xe0\x9f\xbf", b"\xed\xa0\x80",
                b"\xf0\x8f\xbf\xbf", b"\xf4\x90\x80\x80", b"\xf8\x88\x80\x80\x80"]:
        names.update([raw, b"a" + raw, b"p:" + raw])
    for raw in ["é".encode(), "中".encode(), "🙂".encode()]:
        names.update(raw[:size] for size in range(1, len(raw)))
    atoms = list("azAZ_09-.: &\t\n\r<>é中🙂\u0300\u037e\u037f")
    for _ in range(1500):
        names.add("".join(rng.choices(atoms, k=rng.randrange(1, 32))).encode())
    return sorted(names)


def main():
    name = reference()
    names = fixtures()
    expected = []
    for raw in names:
        valid = name(raw)
        parts = raw.split(b":")
        qualified = len(parts) <= 2 and all(name(part) for part in parts)
        expected.append([valid, valid and b":" not in raw, qualified])

    requests = [f"{api} {kind} {raw.hex()}\n"
                for api in range(2) for raw in names for kind in range(3)]
    result = subprocess.run([sys.argv[1], "--oracle"], input="".join(requests),
                            capture_output=True, text=True, check=True)
    lines = result.stdout.splitlines()
    if len(lines) != len(requests):
        raise AssertionError((len(lines), len(requests), result.stderr))

    for index, line in enumerate(lines):
        status, offset = map(int, line.split())
        case = index // 3 % len(names)
        kind = index % 3
        if (status == 0) != expected[case][kind]:
            raise AssertionError((index, kind, names[case], status, expected[case][kind]))
        if status == 0 and offset != len(names[case]):
            raise AssertionError((index, offset, len(names[case])))
    print(f"libxml2 name reference: {len(names)} spans, {len(lines)} inline/compiled checks passed")


if __name__ == "__main__":
    main()
