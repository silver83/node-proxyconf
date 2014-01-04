#include "sysconf_api.h"

// view current config : /Library/Preferences/SystemConfiguration/preferences.plist
// $primary_service -> State/Network/Global/IPv4/PrimaryService
// Setup/Network/Service/$primary_service/proxies

uint32_t ProxySetConfig(const char* ip, uint32_t port, uint32_t enabled) {
  /* get authorized... */
  CFNumberRef portNum, enabledNum;
  portNum = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &port);
  enabledNum = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &enabled);

  AuthorizationRef auth = nil;
  AuthorizationFlags rootFlags = kAuthorizationFlagDefaults
                              | kAuthorizationFlagExtendRights
                              | kAuthorizationFlagInteractionAllowed
                              | kAuthorizationFlagPreAuthorize;


  OSStatus authErr = AuthorizationCreate(nil, kAuthorizationEmptyEnvironment, rootFlags, &auth);
  if ( authErr != noErr ) {
    NSLog(CFSTR("failed auth..."));
    return -1;
  }

  CFStringRef ipCFStr = CFStringCreateWithCStringNoCopy(kCFAllocatorDefault, ip,kCFStringEncodingASCII, kCFAllocatorDefault);

  /* get existing config object */
  SCPreferencesRef prefs = SCPreferencesCreateWithAuthorization(NULL, CFSTR("node-sysconf"), NULL, auth);

  CFStringRef primaryServiceId = GetPrimaryService();

  /* go through another api to fetch current main service */
  SCNetworkServiceRef primaryService = SCNetworkServiceCopy(prefs, primaryServiceId);

  SCNetworkProtocolRef proxyProtoRef = SCNetworkServiceCopyProtocol(primaryService, kSCNetworkProtocolTypeProxies);
  CFDictionaryRef proxyConf = SCNetworkProtocolGetConfiguration(proxyProtoRef);

  /* create updated config object */
  CFMutableDictionaryRef newProxyConf = CFDictionaryCreateMutableCopy(NULL, 0, proxyConf);
  CFDictionarySetValue(newProxyConf, CFSTR("HTTPEnable"), enabledNum);
  CFDictionarySetValue(newProxyConf, CFSTR("HTTPProxy"), ipCFStr);
  CFDictionarySetValue(newProxyConf, CFSTR("HTTPPort"), portNum);

  NSLog(CFSTR("config was : %@"), newProxyConf);

  /* update the system configuration */
  Boolean result;
  result = SCNetworkProtocolSetConfiguration(proxyProtoRef, newProxyConf);
  NSLog(CFSTR("SCNetworkProtocolSetConfiguration : %d"), result);

  result = SCPreferencesCommitChanges(prefs);
  NSLog(CFSTR("SCPreferencesCommitChanges : %d"), result);

  result = SCPreferencesApplyChanges(prefs);
  NSLog(CFSTR("SCPreferencesApplyChanges : %d"), result);

  /* clean up after yourself... */
  CFRelease(prefs);
  CFRelease(primaryServiceId);
  CFRelease(primaryService);
  CFRelease(proxyProtoRef);
  CFRelease(proxyConf);
  CFRelease(newProxyConf);

  AuthorizationFree(auth, kAuthorizationFlagDefaults);

  return (uint32_t)result;
}

void ProxyPrintConfig() {
  SCPreferencesRef prefs = SCPreferencesCreate(NULL, CFSTR("node-sysconf"), NULL);

  CFDictionaryRef proxyConf = ProxyGetConfig(prefs);
  NSLog(CFSTR("ipv4 config is %@"), proxyConf);

  CFRelease(prefs);
  /* with proxy :
  {
      ExceptionsList =     (
          "*.local",
          "169.254/16",
          localhost,
          "127.0.0.1"
      );
      FTPPassive = 1;
      HTTPEnable = 1; // 0 if disabled
      HTTPPort = 9090;
      HTTPProxy = "127.0.0.1";
  } */
}

// get the primary network proxy config from SCNetworkConfiguration
CFDictionaryRef ProxyGetConfig(SCPreferencesRef prefs) {
  CFStringRef primaryServiceId = GetPrimaryService();
  SCNetworkServiceRef primaryService = SCNetworkServiceCopy(prefs, primaryServiceId);

  SCNetworkProtocolRef proxyRef = SCNetworkServiceCopyProtocol(primaryService, kSCNetworkProtocolTypeProxies);
  CFDictionaryRef proxyConf = SCNetworkProtocolGetConfiguration(proxyRef);

  CFRelease(primaryServiceId);
  CFRelease(primaryService);
  CFRelease(proxyRef);
  CFRelease(proxyConf);

  return proxyConf;
}

// get the name of the primary network service from SCPreferences dynamic store dictionary.
// see SCExplorer app to view the possible keys in a tree
CFStringRef GetPrimaryService() {
  SCDynamicStoreRef store;
  CFStringRef  key;
  CFDictionaryRef  globalDict;
  CFStringRef  primaryService = NULL;

  store = SCDynamicStoreCreate(NULL, CFSTR("node-sysconf"), NULL, NULL);
  if (!store) {
  // shouldn't happen but...
  }

  key = SCDynamicStoreKeyCreateNetworkGlobalEntity(NULL, kSCDynamicStoreDomainState, kSCEntNetIPv4);
  globalDict = (CFDictionaryRef)SCDynamicStoreCopyValue(store, key);
  CFRelease(key);
  if (globalDict) {
    primaryService = (CFStringRef)CFDictionaryGetValue(globalDict, kSCDynamicStorePropNetPrimaryService);
    if (primaryService) {
      CFRetain(primaryService);
    }
    CFRelease(globalDict);
  }

  CFRelease(store);
  return primaryService;
}

//TODO: Nothing here is properly released
void SCNetworkPrint() {
  SCPreferencesRef prefs = SCPreferencesCreate(NULL, CFSTR("node-sysconf"), NULL);

  CFArrayRef netifs = SCNetworkInterfaceCopyAll();
  CFIndex netifCount = CFArrayGetCount(netifs);
  for (CFIndex i=0; i < netifCount; i++) {
    SCNetworkInterfaceRef netif = (SCNetworkInterfaceRef)CFArrayGetValueAtIndex(netifs, i);
    CFStringRef netifName = SCNetworkInterfaceGetBSDName(netif);
    CFDictionaryRef netifConfig = SCNetworkInterfaceGetConfiguration(netif);

    NSLog(CFSTR("%@ : %@"), netifName, netifConfig);
  }

  CFArrayRef netsets = SCNetworkSetCopyAll(prefs);
  CFIndex netsetCount = CFArrayGetCount(netsets);
  for (CFIndex i=0; i < netsetCount; i++) {
    SCNetworkSetRef netset = (SCNetworkSetRef)CFArrayGetValueAtIndex(netsets, i);
    CFStringRef netsetName = SCNetworkSetGetName(netset);

    NSLog(CFSTR("netset : %@"), netsetName);
  }

  SCNetworkSetRef currentset = (SCNetworkSetRef)SCNetworkSetCopyCurrent(prefs);
  CFStringRef currentsetName = SCNetworkSetGetName(currentset);
  NSLog(CFSTR("current netset : %@"), currentsetName);

  // list services in current network set
  CFArrayRef services = SCNetworkSetCopyServices(currentset);
  CFIndex serviceCount = CFArrayGetCount(services);
  for (CFIndex i=0; i < serviceCount; i++) {
    SCNetworkServiceRef service = (SCNetworkServiceRef)CFArrayGetValueAtIndex(services, i);
    CFStringRef serviceName = SCNetworkServiceGetName(service);
    NSLog(CFSTR("service : %@"), serviceName);


    // list protocols in current service
    CFArrayRef protocols = SCNetworkServiceCopyProtocols(service);
    CFIndex protocolCount = CFArrayGetCount(protocols);
    for (CFIndex j=0; j < protocolCount; j++) {
      SCNetworkProtocolRef protocol = (SCNetworkProtocolRef)CFArrayGetValueAtIndex(protocols, j);
      CFStringRef protocolTypeName = SCNetworkProtocolGetProtocolType(protocol);

      // show the protocol config
      CFDictionaryRef protocolConfig = SCNetworkProtocolGetConfiguration(protocol);
      NSLog(CFSTR("\%@: \n%@\n\n"), protocolTypeName, protocolConfig);
    }
  }
}