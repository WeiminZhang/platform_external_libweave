{
  'targets': [
    {
      'target_name': 'weave_daemon_examples',
      'type': 'none',
      'dependencies': [
        'sample/daemon.gyp:weave_daemon_sample',
        'light/daemon.gyp:weave_daemon_light',
        'lock/daemon.gyp:weave_daemon_lock',
        'ledflasher/daemon.gyp:weave_daemon_ledflasher',
        'speaker/daemon.gyp:weave_daemon_speaker'
      ]
    }
  ]
}
