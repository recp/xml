# Copyright (c), Recep Aslantas.
#
# MIT License (MIT), http://opensource.org/licenses/MIT
# Full license can be found in the LICENSE file

"""Check raw comment/PI/declaration intervals against Expat callback positions."""

from itertools import product
import subprocess
import sys
import xml.parsers.expat as expat


def fixtures():
    declarations = [b'', b"<?xml version='1.0'?>", b'\xef\xbb\xbf',
                    b'\xef\xbb\xbf<?xml \tversion="1.0" encoding=\'UTF-8\' ?>']
    comments = [b'', b'a', b'<tag> ?x &amp;', b'\r\n\t\xcf\x80', b'x-y > y-z']
    targets = [b'p', b'xml-stylesheet', b'\xcf\x80', b'xml:note']
    data = [b'', b' > < &unknown;', b'?x <!--fake--> <![CDATA[z]]>', b'?']
    cdata = [b'', b'<?p?><!--fake-->', b']', b'] ]> < >']
    sources = []
    for declaration, comment, target, value, literal in product(declarations, comments, targets, data, cdata):
        instruction = b'<?' + target + (b' ' + value if value else b'') + b'?>'
        note = b'<!--' + comment + b'-->'
        sources.append(declaration + note + instruction + b"<r a='&lt;?p?> &gt;'>"
                       + b'<![CDATA[' + literal + b']]>' + instruction + b'<x>' + note
                       + b'</x>&lt;!--fake--></r>' + note + instruction)
    for length in [31, 32, 63, 64, 65, 255, 256, 4096]:
        sources.extend([b'<r><!--' + b'>' * length + b'--></r>',
                        b'<r><?p ' + b'>' * length + b'?></r>',
                        b'<r><![CDATA[' + b'>' * length + b']]></r>',
                        b'<r><!--' + b'-x' * length + b'--></r>',
                        b'<r><?p ' + b'?' * length + b'?></r>',
                        b'<r><![CDATA[' + b']' * length + b']]></r>'])
    for offset in [4094, 4095, 4096, 4097, 8190, 8191, 8192, 8193]:
        sources.append(b'<r>' + b'x' * (offset - 3) + b'<!--x--><?p?></r>')
    return sources


def reference(source):
    parser = expat.ParserCreate()
    spans = []

    def record(kind, closing):
        offset = parser.CurrentByteIndex
        end = source.index(closing, offset + (4 if kind == 0 else 2)) + len(closing)
        spans.append((kind, offset, end - offset))

    parser.CommentHandler = lambda text: record(0, b'-->')
    parser.ProcessingInstructionHandler = lambda target, data: record(1, b'?>')
    parser.XmlDeclHandler = lambda version, encoding, standalone: record(2, b'?>')
    parser.Parse(source, True)
    return spans


def main():
    sources = fixtures()
    expected = [reference(source) for source in sources]
    result = subprocess.run([sys.argv[1], '--oracle'], input=''.join(source.hex() + '\n' for source in sources),
                            capture_output=True, text=True, check=True)
    groups = result.stdout.split('BEGIN\n')[1:]
    if len(groups) != len(sources):
        raise AssertionError((len(groups), len(sources), result.stderr))
    count = 0
    for index, group in enumerate(groups):
        lines = group.splitlines()
        actual = [tuple(map(int, line.split())) for line in lines[:-1]]
        if actual != expected[index] or lines[-1] != f'END {len(sources[index])}':
            raise AssertionError((index, sources[index], actual, expected[index], lines[-1]))
        count += len(actual)
    print(f'{expat.EXPAT_VERSION} markup intervals: {len(sources)} documents, {count} exact spans passed')


if __name__ == '__main__':
    main()
