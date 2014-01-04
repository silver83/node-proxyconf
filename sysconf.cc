#include <node.h>
#include <string>
#include "sysconf_api.h"

using namespace v8;
using namespace std;

Handle<Value> SetConfig(const Arguments& args) {
  HandleScope scope;

  if (!args[0]->IsString() || !args[1]->IsNumber() || !args[2]->IsNumber()) {
    ThrowException(Exception::TypeError(String::New("Wrong arguments")));
    return scope.Close(Undefined());
  }

  v8::String::Utf8Value ipUtf8String(args[0]->ToString());  // 1. get arg as v8 string
  std::string ipStdString = std::string(*ipUtf8String);     // 2. convert to c++ string
  const char* ipCstr = ipStdString.c_str();

  uint32_t port, enabled;
  port = args[1]->Uint32Value();
  enabled = args[2]->Uint32Value();
  int oss = ProxySetConfig(ipCstr, port, enabled);

  if (oss) return scope.Close(String::New("proxy set successfully!"));
  return scope.Close(String::New("proxy failed setting configuration!"));
}

void Init(Handle<Object> exports) {
  exports->Set(String::NewSymbol("proxySetConfig"),
      FunctionTemplate::New(SetConfig)->GetFunction());
}

NODE_MODULE(sysconf, Init)