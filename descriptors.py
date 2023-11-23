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

descriptors = {
    'class-proto':      (0x00, 0x00, 0x00),
    'max-packet-size':  64,
    'ids':              (0x1209, 0x0001),
    'device-version':   (1, 0),
    'manufacturer':     'metebalci',
    'product':          'hal5',
    'serial':           '007',
    'configurations':
    [
        {
            'value':    1,
            'label':    'configuration 1',
            'self-powered':     True,
            'remote-wakeup':    False,
            'max-power-ma':     0,
            'interfaces':
            [
                {
                    'number':   0,
                    'label':    'interface 1',
                    'alternate-setting': 0,
                    'class-proto':  (0x00, 0x00, 0x00),
                    'endpoints':
                    [
                        {
                            'address':          1,
                            'direction':        'in',
                            'transfer-type':    'iso',
                            'sync-type':        'no syncronization',
                            'usage-type':       'data endpoint',
                            'max-packet-size':  64,
                            'interval':         1
                        },
                    ]
                },
            ]
        },
    ]
}
