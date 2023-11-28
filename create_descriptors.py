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

from math import ceil
import sys

from descriptors import descriptors

strings = ['LANGID en-US']
encoded_strings = [b'\x09\x04']

def encode_string(s, padding=0):
    if s is None or len(s) == 0:
        return 0
    idx = len(strings)
    s = s + ' ' * padding
    strings.append(s)
    encoded_string = s.encode('utf-16le')
    encoded_strings.append(encoded_string)
    return idx

indent = 0

def p(*args, **kwargs):
    if 'donotindent' not in kwargs:
        print(' ' * indent, end='')
    else:
        del kwargs['donotindent']
    print(*args, **kwargs)

def tab():
    global indent
    indent+=4

def untab():
    global indent
    indent-=4

def create_string_descriptor(i):
    p('static hal5_usb_string_descriptor_t hal5_usb_string_descriptor_%d = ' % i)
    p('{')
    tab()
    # size
    p('%d,' % (2+len(encoded_strings[i])))
    p('0x03,') # descriptor type
    p('// "%s"' % strings[i])
    p('{', end='')
    for k in range(0, len(encoded_strings[i])):
        if k > 0:
            p(', ', end='', donotindent=True)
        p('0x%02X' % encoded_strings[i][k], end='', donotindent=True)
    p('},', donotindent=True)
    untab()
    p('};')

def create_string_descriptors():
    global strings
    global encoded_strings
    # USB 2.0 Spec 9.6.7: USB devices that omit all string descriptors
    # must not return an array of LANGID codes
    if len(strings) == 1:
        strings[0] = ''
        encoded_strings[0] = ''

    # individual string descriptors
    for i in range(0, len(strings)):
        create_string_descriptor(i)
    # end

    # number of string descriptors
    p('const uint32_t hal5_usb_number_of_string_descriptors = %d;' % len(strings))

    # array
    p('const hal5_usb_string_descriptor_t* const hal5_usb_string_descriptors[] = ')
    p('{')
    tab()
    for i in range(0, len(strings)):
        p('&hal5_usb_string_descriptor_%d,' % i)
    untab()
    p('};')

def create_endpoint_descriptor(d):
    address = d['address']
    assert address != 0, 'endpoint address cannot be 0'
    assert address < 16, 'endpoint address has to be < 16'
    p('static const hal5_usb_endpoint_descriptor_t hal5_usb_endpoint_descriptor_%d = ' % address)
    p('{')
    tab()
    p('7, // bLength')
    p('0x05, // bDescriptorType')
    tt = d['transfer-type'].lower()[0:3]
    # for control, direction is ignored (set to zero=out)
    if tt == 'con':
        assert 'direction' not in d, 'direction given but the endpoint is control'
    else:
        assert 'direction' in d, 'direction not given but the endpoint is not control'
        if d['direction'].lower() == 'in':
            address = 0x80 | address
    p('0x%02X, // bEndpointAddress' % address)
    attr = 0
    # if tt is iso, st and ut is checked
    # if tt is not iso, st and ut are reserved to be 0
    if tt == 'con':
        attr = attr | 0x00
        assert 'sync-type' not in d, 'sync-type given but the endpoint is control'
        assert 'usage-type' not in d, 'usage-type given but the endpoint is control'
    elif tt == 'iso':
        attr = attr | 0x01
        st = d['sync-type'].lower()[0:2]
        if st == 'no':      # e.g. no sync
            attr = attr | 0x00
        elif st == 'as':    # e.g. async
            attr = attr | 0x04
        elif st == 'ad':    # e.g. adapt
            attr = attr | 0x08
        elif st[0] == 's':  # e.g. sync
            attr = attr | 0x0C
        else:
            assert False, 'wrong endpoint sync type: %s' % (d['sync-type'])
        ut = d['usage-type'].lower()[0]
        if ut == 'd':       # e.g. data endpoint
            attr = attr | 0x00
        elif ut == 'f':     # e.g. feedback endpoint
            attr = attr | 0x10
        elif ut == 'i':     # e.g. implicit feedback data endpoint
            attr = attr | 0x20
        else:
            assert False, 'wrong endpoint usage type: %s' % (d['usage-type'])
    elif tt == 'bul':
        attr = attr | 0x02
        assert 'sync-type' not in d, 'sync-type given but the endpoint is bulk'
        assert 'usage-type' not in d, 'usage-type given but the endpoint is bulk'
    elif tt == 'int':
        attr = attr | 0x03
        assert 'sync-type' not in d, 'sync-type given but the endpoint is interrupt'
        assert 'usage-type' not in d, 'usage-type given but the endpoint is interrupt'
    else:
        assert False, 'wrong endpoint transfer type: %s' % (d['transfer-type'])
    p('0x%02X, // bmAttributes' % attr)
    wMaxPacketSize = d['max-packet-size']
    # STM32H5 is USB FS
    # so max is max of USB FS which is 1023 bytes
    assert wMaxPacketSize <= 1023, 'wMaxPacketSize should be <= 1023'
    assert wMaxPacketSize % 4 == 0, 'not a must but highly recommended to make wMaxPacketSize a multiple of 4'
    p('%d, // wMaxPacketSize' % d['max-packet-size'])
    if tt == 'iso' or tt == 'int':
        p('%d, // bInterval' % d['interval'])
    else:
        assert 'interval' not in d, 'interval is given but the endpoint is neither iso nor interrupt'
    untab()
    p('};')

def create_interface_descriptor(d):
    for endpoint in d['endpoints']:
        create_endpoint_descriptor(endpoint)
    p('static const hal5_usb_interface_descriptor_t hal5_usb_interface_descriptor_%d = ' % d['number'])
    p('{')
    tab()
    cp = d['class-proto']
    p('9, // bLength')
    p('0x04, // bDescriptorType')
    p('%d, // bInterfaceNumber' % d['number'])
    p('%d, // bAlternateSetting' % d['alternate-setting'])
    p('%d, // bNumEndpoints' % len(d['endpoints']))
    p('0x%02X, // bInterfaceClass' % cp[0])
    p('0x%02X, // bInterfaceSubClass' % cp[1])
    p('0x%02X, // bInterfaceProtocol' % cp[2])
    p('%d, // iInterface' % encode_string(d['label']));
    p('{')
    tab()
    for endpoint in d['endpoints']:
        p('&hal5_usb_endpoint_descriptor_%d, ' % endpoint['address'])
    untab()
    p('},')
    untab()
    p('};')

def create_configuration_descriptor(idx, d):
    for interface in d['interfaces']:
        create_interface_descriptor(interface)
    p('static const hal5_usb_configuration_descriptor_t hal5_usb_configuration_descriptor_%d = ' % idx)
    p('{')
    tab()
    p('9, // bLength')
    p('0x02, // bDescriptorType')
    total_length = 9
    for i in d['interfaces']:
        total_length = total_length + 9
        for e in i['endpoints']:
            total_length = total_length + 7
    p('%d, // wTotalLength' % total_length)
    p('%d, // bNumInterfaces' % len(d['interfaces']))
    p('%d, // bConfigurationValue' % d['value'])
    p('%d, // iConfiguration' % encode_string(d['label']));
    # attr[7] reserved one
    # attr[4:0] reserved zero
    attr = 0x80
    if d['self-powered']:
        attr = attr | 0x40
    if d['remote-wakeup']:
        attr = attr | 0x20
    p('0x%02X, // bmAttributes' % attr)
    p('0x%02X, // bMaxPower' % ceil(d['max-power-ma']/2.0))
    p('{')
    tab()
    for interface in d['interfaces']:
        p('&hal5_usb_interface_descriptor_%d, ' % interface['number'])
    untab()
    p('},')
    untab()
    p('};')

def create_device_descriptor(d):
    for i in range(0, len(d['configurations'])):
        conf = d['configurations'][i]
        create_configuration_descriptor(i, conf)
    p('static const hal5_usb_device_descriptor_t hal5_usb_device_descriptor_0 =')
    p('{')
    tab()
    cp = d['class-proto']
    ids = d['ids']
    dv = d['device-version']
    p('18, // bLength')
    p('0x01, // bDescriptorType')
    p('0x0200, // bcdUSB')
    p('0x%02X, // bDeviceClass' % cp[0])
    p('0x%02X, // bDeviceSubClass' % cp[1])
    p('0x%02X, // bDeviceProtocol' % cp[2])
    bMaxPacketSize0 = d['max-packet-size-ep0']
    assert (bMaxPacketSize0 == 8 or
            bMaxPacketSize0 == 16 or
            bMaxPacketSize0 == 32 or
            bMaxPacketSize0 == 64), 'bMaxPacketSize0 has to be 8,16,32 or 64 and depends on USB speed'
    p('%d, // bMaxPacketSize0' % bMaxPacketSize0)
    p('0x%04X, // idVendor' % ids[0])
    p('0x%04X, // idProduct'  % ids[1])
    p('0x%02X%02X, // bcdDevice' % (dv[0], dv[1]))
    p('%d, // iManufacturer' % encode_string(d['manufacturer']))
    version_padding = 0
    if d.get('append_version', True):
        version_padding = len(' v00.00')
    p('%d, // iProduct' % encode_string(d['product'], padding=version_padding))
    p('%d, // iSerialNumber' % encode_string(d['serial']))
    p('%d, // bNumConfigurations' % len(d['configurations']))
    p('{')
    tab()
    for i in range(0, len(d['configurations'])):
        p('&hal5_usb_configuration_descriptor_%d, ' % i)
    untab()
    p('},')
    untab()
    p('};')
    p('const hal5_usb_device_descriptor_t* const hal5_usb_device_descriptor = &hal5_usb_device_descriptor_0;')
    if d.get('append_version', True):
        p('const bool hal5_usb_product_string_append_version = true;')
    else:
        p('const bool hal5_usb_product_string_append_version = false;')

def create_descriptors(d):
    p('#include <stdbool.h>')
    p('#include "hal5_usb.h"')
    create_device_descriptor(d)
    create_string_descriptors()

create_descriptors(descriptors)
