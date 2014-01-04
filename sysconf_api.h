#ifndef __HELPERS_H_
#define __HELPERS_H_

#include <CoreFoundation/CoreFoundation.h>
#include <SystemConfiguration/SystemConfiguration.h>

#if __cplusplus
extern "C" {
#endif
void NSLog(CFStringRef format, ...);
void NSLogv(CFStringRef format, va_list args);
#if __cplusplus
}
#endif

void SCNetworkPrint();
CFDictionaryRef ProxyGetConfig(SCPreferencesRef prefs);
void ProxyPrintConfig();

CFStringRef GetPrimaryService();
uint32_t ProxySetConfig(const char* ip, uint32_t port, uint32_t enabled);

#endif