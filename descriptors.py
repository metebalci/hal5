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

# descriptors is the main dictionary, used by create_descriptors.py
# its name has to be descriptors
# it has a tree like structure, children are stored in arrays

# because this is a Python file (not JSON), hex numbers can be used

# there are no explicit string descriptors here because all strings are given
# inline and string descriptors are automatically created from them

# configuration, interface and endpoint descriptions can also be embedded
# to this dictionary. I split them up below into their own dictionaries because
# it is easier to work like this during development
descriptors = {
    # the first level is device descriptor (since there is only one)

    # (bDeviceClass, bDeviceSubClass, bDeviceProtocol)
    # 0 means it is given in the interface descriptors
    'class-proto':      (0x00, 0x00, 0x00),

    # bMaxPacketSize0
    # for USB LS, it should be 8
    # for USB FS, it can be any of 8,16,32 or 64
    # for USB HS, it should be 64
    'max-packet-size-ep0':  64,

    # (idVendor, idProduct)
    'ids':              (0x1209, 0x0001),

    # bcdDevice (major, minor)
    'device-version':   (1, 0),

    # iManufacturer, None or String
    'manufacturer':     'metebalci',

    # iProduct, None or String
    'product':          'hal5',

    # if True, append the runtime version to product string above
    'append_version':   True,

    # iSerialNumber
    'serial':           None,

    # configurations
    'configurations':   []
}

configuration0 = {
    # bConfigurationValue
    'value':    1,

    # iConfiguration
    'label':    'configuration 1',

    # bmAttributes Self-powered (D6) bit
    'self-powered':     True,

    # bmAttributes Remote Wakeup (D5) bit
    'remote-wakeup':    False,

    # bMaxPower but specified in mA
    # actual value used in the descriptor is half of the value here
    # since USB descriptor is in 2mA units
    # 0 can be specified for self powered devices
    # then get_status should always report self powered
    'max-power-ma':     0,

    # interfaces of this configuration
    'interfaces': []
}

interface0 = {
    # bInterfaceNumber
    # although there is no need to specify this explicitly (the order in
    # interfaces array can be used), this is an important value
    # so I decided to keep it explicit
    'number':   0,

    # iInterface
    'label':    'interface 0',

    # bAlternateSetting
    'alternate-setting': 0,

    # (bInterfaceClass, bInterfaceSubClass, bInterfaceProtocol)
    # 0xFF means it is vendor-specific
    'class-proto': (0xFF, 0xFF, 0xFF),

    # endpoints
    'endpoints':
    [
        {
            # bEndpointAddress
            'address':          1,

            # direction, encoded in actual bEndpointAddress value
            # not required for control endpoints
            #'direction':        'in',

            # bmAttributes Transfer Type (Bits 1..0)
            # control, iso, bulk or interrupt
            'transfer-type':    'control',

            # bmAttributes Synchronization Type (Bits 3..2)
            # no-sync, async, adaptive or sync
            # required only for iso endpoints
            #'sync-type':    'no-sync',

            # bmAttributes Usage Type (Bits 5..4)
            # data, feedback, implicit feedback data
            # required only for iso endpoints
            #'usage-type':    'data',

            # wMaxPacketSize
            'max-packet-size':  64,

            # bInterval, required only for iso and interrupt endpoints
            #'interval':         1
        }
    ]
}

interface1 = {
    'number':   1,
    'label':    'interface 1',
    'alternate-setting': 0,
    'class-proto': (0xFF, 0xFF, 0xFF),
    'endpoints':
    [
        {
            'address':          2,
            'direction':        'in',
            'transfer-type':    'bulk',
            'max-packet-size':  64,
            'buffer-size':      64,
        },
        {
            'address':          3,
            'direction':        'out',
            'transfer-type':    'bulk',
            'max-packet-size':  64,
            'buffer-size':      64,
        },
    ]
}

# this function sets the mps to particular value
# and then sets the string descriptors to some mps related
# values so the transfers can be tested
def testmps(mps):
    def genstr(packet_size):
        assert (packet_size % 2 == 0)
        strlen = packet_size - 2
        strlenx = strlen >> 1
        return ' ' * strlenx

    mps = 64
    descriptors['max-packet-size-ep0'] = mps
    descriptors['append_version'] = False
    descriptors['manufacturer'] = genstr(mps - 2)
    descriptors['product'] = genstr(mps)
    descriptors['serial'] = genstr(mps + 2)
    configuration0['label'] = genstr(mps * 2)
    interface0['label'] = genstr(mps * 2 + 2)

descriptors['configurations'].append(configuration0)
configuration0['interfaces'].append(interface0)
#configuration0['interfaces'].append(interface1)
