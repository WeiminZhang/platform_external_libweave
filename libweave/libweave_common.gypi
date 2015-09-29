{
  'target_defaults': {
    'configurations': {
      'Release': {
        'defines': [
          'NDEBUG',
        ],
        'cflags': [
          '-Os',
        ],
      },
      'Debug': {
        'defines': [
          '_DEBUG',
        ],
        'cflags': [
          '-Og',
        ],
      },
    },
    'include_dirs': [
      '.',
      'include',
      'external',
      'third_party/include',
      'third_party/modp_b64/modp_b64',
    ],
    'cflags!': ['-fPIE'],
    'cflags': [
      '-fPIC',
      '-fvisibility=hidden',
      '-std=c++11',
      '-Wall',
      '-Werror',
      '-Wextra',
      '-Wl,--exclude-libs,ALL',
      '-Wno-char-subscripts',
      '-Wno-format-nonliteral',
      '-Wno-missing-field-initializers',
      '-Wno-unused-local-typedefs',
      '-Wno-unused-parameter',
      '-Wpacked',
      '-Wpointer-arith',
      '-Wwrite-strings',
    ],
    'libraries': [
      '-L../../third_party/lib',
    ],
  },
}
