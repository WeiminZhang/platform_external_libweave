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
          'libnl-3.0',
          'libnl-route-3.0',
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
        'netlink_network.cc',
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
