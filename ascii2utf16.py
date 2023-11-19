#!/usr/bin/python3

#
# SPDX-FileCopyrightText: 2023 Mete Balci
#
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2023 Mete Balci
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

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
