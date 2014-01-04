{
  "targets": [
    {
      "target_name": "sysconf",
      "sources": [ "sysconf.cc", "sysconf_api.cc" ]
    }
  ],
  'link_settings': {
    'libraries': [
      '$(SDKROOT)/System/Library/Frameworks/SystemConfiguration.framework',
      '$(SDKROOT)/System/Library/Frameworks/CoreFoundation.framework'
    ],
  }
}