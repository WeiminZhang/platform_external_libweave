# Copyright 2015 The Weave Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
{
  'targets': [
    {
      'target_name': 'weave',
      'type': 'executable',
      'variables': {
        'deps': [
          'avahi-client',
          'expat',
          'libcurl',
          'libcrypto',
          'openssl',
        ]
      },
      'cflags': [
        '>!@(pkg-config >(deps) --cflags)',
        '-pthread',
      ],
      'link_settings': {
        'ldflags+': [
          '>!@(pkg-config >(deps) --libs-only-L --libs-only-other)',
        ],
        'libraries+': [
          '>!(pkg-config >(deps) --libs-only-l)',
        ],
      },
      'sources': [
        'avahi_client.cc',
        'bluez_client.cc',
        'curl_http_client.cc',
        'event_http_client.cc',
        'event_http_server.cc',
        'event_task_runner.cc',
        'file_config_store.cc',
        'main.cc',
        'event_network.cc',
        'network_manager.cc',
        'ssl_stream.cc',
      ],
      'dependencies': [
        '../../libweave_standalone.gyp:libweave',
      ],
      'libraries': [
        '-levent',
        '-levent_openssl',
        '-lpthread',
      ]
    }
  ]
}
