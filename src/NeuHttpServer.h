#ifndef NEU_HTTP_SERVER
#define NEU_HTTP_SERVER

#if !defined(ESP32)
#error "Platform not supported! NeuHttpServer currently only supports ESP32."
#endif

#include "http/httpServer/httpServer.h"
#include "dns/dnsServer.h"

#endif