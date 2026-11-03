"""Compare raw value decoding with the independent stdlib Expat reader."""

import random
import subprocess
import sys
import xml.etree.ElementTree as ET


def reference(mode, raw):
    if mode == 0:
        document = b"<r>" + raw + b"</r>"
    elif mode == 1:
        document = b'<r a="' + raw + b'"/>'
    else:
        document = b"<r><![CDATA[" + raw + b"]]></r>"

    try:
        root = ET.fromstring(document)
    except ET.ParseError:
        return None
    value = root.attrib["a"] if mode == 1 else root.text or ""
    return value.encode("utf-8")


def cases():
    rng = random.Random(74813)
    common = [b"", b"ascii", b" ", b"\t", b"\n", b"\r", b"\r\n", b">", b"]",
              "Türkçe 🙂".encode(), "\u0085\u2028".encode()]
    references = [b"&amp;", b"&lt;", b"&gt;", b"&apos;", b"&quot;",
                  b"&#9;", b"&#10;", b"&#13;", b"&#x000041;", b"&amp;lt;"]
    scalars = [0, 8, 9, 10, 11, 12, 13, 14, 31, 32, 127, 128, 2047, 2048,
               0xD7FF, 0xD800, 0xDFFF, 0xE000, 0xFFFD, 0xFFFE, 0xFFFF,
               0x10000, 0x1FFFE, 0x10FFFF, 0x110000]
    scalars += [rng.randrange(0x110001) for _ in range(1500)]
    invalid = [b"&", b"&missing;", b"&AMP;", b"&#;", b"&#x;", b"&#X41;",
               b"&#-1;", b"&#+9;", b"&# 9;", b"&#9 ;", b"&#xag;",
               b"&#999999999999999999999;", b"&#xffffffff;", b"x<",
               b"\xc0\x80", b"\xe0\x9f\xbf", b"\xed\xa0\x80",
               b"\xf0\x8f\xbf\xbf", b"\xf4\x90\x80\x80"]

    for mode in range(3):
        atoms = common + references
        if mode == 2:
            atoms += [b"<", b"&missing;", b"<![CDATA["]
        for raw in atoms:
            yield mode, raw
        for _ in range(1200):
            yield mode, b"".join(rng.choices(atoms, k=rng.randrange(1, 32)))
        for value in scalars:
            yield mode, f"&#{value};".encode()
            yield mode, f"&#x{value:x};".encode()
            if value <= 0x10FFFF and not 0xD800 <= value <= 0xDFFF:
                raw = chr(value).encode()
                # Quote delimiters and nested markup belong to document grammar.
                if raw not in (b'"', b"<", b"&"):
                    yield mode, raw
        for raw in invalid:
            yield mode, raw
        for value in range(256):
            raw = bytes([value])
            if raw not in (b'"', b"<"):
                yield mode, b"prefix" + raw
        for raw in references + ["é".encode(), "中".encode(), "🙂".encode()]:
            for length in range(1, len(raw)):
                yield mode, raw[:length]
        yield mode, b"&#" + b"0" * 3000 + b"9;"
        # A raw CDATA terminator could close the wrapper and reopen text; use
        # complete payloads whose enclosing document is necessarily invalid.
        yield mode, b"a]]>b"


def main():
    fixtures = list(cases())
    expected = [reference(mode, raw) for mode, raw in fixtures]
    requests = [f"{api} {mode} {raw.hex()}\n"
                for api in range(2) for mode, raw in fixtures]
    result = subprocess.run([sys.argv[1], "--oracle"], input="".join(requests),
                            capture_output=True, text=True, check=True)
    responses = result.stdout.splitlines()
    if len(responses) != len(requests):
        raise AssertionError((len(responses), len(requests), result.stderr))

    for index, line in enumerate(responses):
        status, offset, output = line.split(" ", 2)
        mode, raw = fixtures[index % len(fixtures)]
        want = expected[index % len(fixtures)]
        actual = bytes.fromhex(output) if int(status) == 0 else None
        if actual != want or (want is not None and int(offset) != len(raw)):
            raise AssertionError((index, mode, raw, status, offset, actual, want))
    print(f"Expat reference: {len(fixtures)} spans, {len(responses)} inline/compiled checks passed")


if __name__ == "__main__":
    main()
