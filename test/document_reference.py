# Copyright (c), Recep Aslantas.
#
# MIT License (MIT), http://opensource.org/licenses/MIT
# Full license can be found in the LICENSE file

"""Compare the checked DTD-free document grammar with Expat after byte edits.

Generic XML Name mode is selected: namespace constraints have separate checks.
Declaration/version/encoding capabilities and Fifth Edition name tables have
their own fixtures; this cohort mutates ASCII document structure and content.
"""

import subprocess
import sys
import xml.parsers.expat as expat


def fixtures():
    seeds = [b'<r/>', b'<r></r>', b'<r><x/><y/></r>', b'<r a="1" b=\'2\'/>',
             b'<r>x&amp;y&#65;</r>', b'<r> a <x/> b </r>',
             b'<!--c--><r/><!--d-->', b'<r><!--a-b--></r>',
             b'<?p?><r/><?q data?>', b'<r><?pi a:b?></r>',
             b'<r><![CDATA[x<&amp;]]></r>', b' \t\r\n<r/> \t\r\n',
             b'<r xmlns="urn:u" xmlns:p="urn:p"><p:n p:a="v"/></r>',
             b'<:r :a="v"/>', b'<a:b:c a:b:c="v"/>']
    cases = {b'', *seeds}
    for seed in seeds:
        for i in range(len(seed) + 1):
            cases.add(seed[:i] + seed[i + 1:])
            for c in range(128):
                byte = bytes([c])
                cases.add(seed[:i] + byte + seed[i:])
                if i < len(seed):
                    cases.add(seed[:i] + byte + seed[i + 1:])
    return sorted(cases)


def main():
    cases = fixtures()
    expected = []
    for source in cases:
        try:
            expat.ParserCreate().Parse(source, True)
            expected.append(True)
        except expat.ExpatError:
            expected.append(False)

    # Compare reversed mutable and normal readonly input, with both cap paths.
    requests = [f'{mode} {bounded} {source.hex() or "-"}\n'
                for source in cases for mode in [1, 4] for bounded in range(2)]
    result = subprocess.run([sys.argv[1], '--oracle'], input=''.join(requests),
                            capture_output=True, text=True, check=True)
    lines = result.stdout.splitlines()
    if len(lines) != len(requests):
        raise AssertionError((len(lines), len(requests), result.stderr))
    for index, line in enumerate(lines):
        status, offset = map(int, line.split())
        source = cases[index // 4]
        if (status == 0) != expected[index // 4] or offset > len(source):
            raise AssertionError((index, source, line, expected[index // 4]))
        if status == 0 and offset != len(source):
            raise AssertionError((index, source, offset))
    print(f'{expat.EXPAT_VERSION} document grammar: {len(cases)} cases, {len(lines)} checks passed')


if __name__ == '__main__':
    main()
