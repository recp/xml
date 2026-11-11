# Copyright (c), Recep Aslantas.
#
# MIT License (MIT), http://opensource.org/licenses/MIT
# Full license can be found in the LICENSE file

"""Check complete source partitions using Expat's token positions."""

import subprocess
import sys
import xml.parsers.expat as expat
from markup_reference import fixtures


def reference(source):
    parser = expat.ParserCreate()
    spans = []
    empty = []
    cdata_start = None

    def tag_end(offset):
        quote = None
        for index in range(offset + 1, len(source)):
            byte = source[index]
            if quote is not None:
                if byte == quote:
                    quote = None
            elif byte in (34, 39):
                quote = byte
            elif byte == 62:
                return index + 1
        raise AssertionError(source)

    def start(name, attributes):
        offset = parser.CurrentByteIndex
        end = tag_end(offset)
        is_empty = source[end - 2:end] == b'/>'
        spans.append((1 if is_empty else 0, offset, end - offset))
        empty.append(is_empty)

    def end(name):
        if not empty.pop():
            offset = parser.CurrentByteIndex
            spans.append((2, offset, tag_end(offset) - offset))

    def record(kind, closing, prefix):
        offset = parser.CurrentByteIndex
        end = source.index(closing, offset + prefix) + len(closing)
        spans.append((kind, offset, end - offset))

    def cdata_begin():
        nonlocal cdata_start
        cdata_start = parser.CurrentByteIndex

    def cdata_end():
        spans.append((4, cdata_start, parser.CurrentByteIndex + 3 - cdata_start))

    parser.StartElementHandler = start
    parser.EndElementHandler = end
    parser.CommentHandler = lambda text: record(5, b'-->', 4)
    parser.ProcessingInstructionHandler = lambda target, data: record(6, b'?>', 2)
    parser.XmlDeclHandler = lambda version, encoding, standalone: record(7, b'?>', 2)
    parser.StartCdataSectionHandler = cdata_begin
    parser.EndCdataSectionHandler = cdata_end
    parser.Parse(source, True)
    if source.startswith(b'\xef\xbb\xbf'):
        spans.append((8, 0, 3))
    result = []
    cursor = 0
    for kind, offset, size in sorted(spans, key=lambda span: span[1]):
        if offset > cursor:
            result.append((3, cursor, offset - cursor))
        result.append((kind, offset, size))
        cursor = offset + size
    if cursor < len(source):
        result.append((3, cursor, len(source) - cursor))
    return result


def main():
    sources = fixtures() + [b" \r\n<r a='>' b=\"/>\"> &amp; <x/><y>z</y>tail</r> \t"]
    expected = [reference(source) for source in sources]
    result = subprocess.run([sys.argv[1], '--oracle'],
                            input=''.join(source.hex() + '\n' for source in sources),
                            capture_output=True, text=True, check=True)
    groups = result.stdout.split('BEGIN\n')[1:]
    assert len(groups) == len(sources), result.stderr
    count = 0
    for index, group in enumerate(groups):
        lines = group.splitlines()
        actual = [tuple(map(int, line.split())) for line in lines[:-1]]
        assert actual == expected[index], (index, sources[index], actual, expected[index])
        assert lines[-1] == f'END {len(sources[index])}'
        count += len(actual)
    print(f'{expat.EXPAT_VERSION}: {len(sources)} documents, {count} source events passed')


if __name__ == '__main__':
    main()
