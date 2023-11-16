#!/usr/bin/python3

import sys

if len(sys.argv) < 2:
    print('ascii2utf16.py <string>')
    sys.exit(0)

s = sys.argv[1]
b = s.encode('utf-16le')

print('{', end='')

for i in range(0, len(b)):
    if i > 0:
        print(', ', end='')
    print(hex(b[i]), end='')

print('}')
