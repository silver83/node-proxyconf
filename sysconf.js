var addon = require('./build/Release/sysconf');

console.log(addon.proxySetConfig('127.0.0.1', 9090, 1));