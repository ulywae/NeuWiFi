# NeuWiFi

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform: Arduino](https://img.shields.io/badge/Platform-Arduino-00878F?logo=arduino&logoColor=white)](https://arduino.cc)
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-blue?logo=espressif&logoColor=white)](https://espressif.com)
[![Architecture: 32-bit](https://img.shields.io/badge/Architecture-32--bit-orange.svg)]()
[![Design: Deterministic](https://img.shields.io/badge/Design-Deterministic-critical.svg)]()

Professional ESP32 Networking Toolkit for ESP-IDF

**NeuWiFi** is a complete networking ecosystem designed exclusively for the ESP32 family, compatible with both Arduino Core and native ESP-IDF environments. It combines high-performance execution with an ultra-lean memory footprint, providing everything you need to build robust IoT applications — from basic connectivity to advanced web services and device discovery.

Unlike generic libraries that simply wrap low-level APIs, NeuWiFi is built around a **centralized state architecture**, ensuring all modules work in perfect synchronization. It prioritizes deterministic behavior, defensive resource management, and minimal overhead, making it suitable for both hobbyist projects and embedded systems.

---

# Features

NeuWiFi packs a full suite of networking capabilities into one cohesive framework:

## Core WiFi Engine

- **STA (Station)**, **SoftAP**, and **Hybrid (AP+STA)** concurrent operation
- Static IP configuration with validation & LwIP endianness correction via `IPConfig`
- DHCP support, custom Hostname setting, TX Power control, and security policy management
- Smart **Auto-Reconnect** logic with failure detection
- Non-blocking WiFi Scan API with detailed result parsing
- Complete **Event-Driven** callback system for state changes
- Automatic status synchronization to the global `SharedState` manager

## Centralized State Management

> _The heart of the NeuWiFi architecture_

- Unified system awareness: All modules (UDP, DNS, HTTP, mDNS) share the same operational state
- Intelligent service lifecycle: Services automatically start/stop when the network comes up or goes down
- **Client Tracking System**: Differentiates "new" vs "authenticated" users — essential for captive portals
- Thread-safe operations with atomic flags and event groups
- Deterministic behavior with zero hidden logic

## UDP Transport Layer

- Lightweight, high-speed socket implementation
- Full support for Unicast, Broadcast, and Multicast communication
- Runs asynchronously in a dedicated FreeRTOS task — independent of the main loop
- Configurable blocking mode, receive timeout, and task stack size
- Dual callback system: Simple payload handler or raw `sockaddr_in` access for advanced use cases

## HTTP Server & WebSocket

- Asynchronous web server built directly on ESP-IDF HTTP Server APIs
- Static content routes, dynamic URI handlers, JSON helpers, and redirect utilities
- POST parameter parser and custom error page handlers
- **Full WebSocket Support**: Persistent connections, text/binary frames, broadcast messaging, and Ping/Pong
- Client management: Get active connections, close clients, or send data directly to specific file descriptors

## DNS Server — Captive Portal Engine

- Port 53 DNS responder built on NeuUDP
- Intelligent response logic: Redirects only unauthenticated clients using data from `SharedState`
- Resolves all domains to device IP — perfect for Wi-Fi onboarding portals
- Extremely low memory footprint and high performance

## mDNS Service Discovery

- Native Multicast DNS implementation to avoid system conflicts
- Hostname advertisement (e.g., `device.local`)
- Service publishing with custom **TXT Records** for metadata
- Discovery mode: Scan the local network for other NeuWiFi or mDNS-enabled devices
- Structured key-value storage with fixed-size safety buffers

## HTTP Client

- HTTP GET
- HTTP POST
- HTTPS support
- Custom headers
- Buffered response handling
- Internal response streaming

## Utility: IP Configuration

- Safe storage class for IPv4 addresses, gateways, and DNS
- Automatic conversion between string format and raw LwIP `ip_addr_t`
- Built-in validation to prevent invalid network configurations
- Used consistently across all modules for type safety

---

# Design Philosophy

NeuWiFi is built around a simple principle:

> If a system can fail unpredictably once, then the runtime behavior is not fully controlled yet.

The library prioritizes:

- deterministic runtime behavior
- bounded buffers
- defensive guards
- cleanup discipline
- async subsystem isolation
- minimal hidden behavior

Most subsystems are event-driven and operate independently from the Arduino-style main loop.
This means networking operations continue to function even when the main loop is heavily delayed.

---

# Platform Support

| Platform | Status        |
| -------- | ------------- |
| ESP32    | Supported     |
| ESP8266  | Not Supported |
| AVR      | Not Supported |
| STM32    | Not Supported |

NeuWiFi is intentionally designed specifically for ESP32.

Different platforms have different networking architectures, runtime models, and SDK behaviors.
This library does not attempt to be cross-platform.

---

# Installation

## ESP-IDF

Clone or copy the library into your project components directory.

```bash
components/
└── NeuWiFi/
```

Then include the required headers.

```cpp
#include "NeuWiFi.h"
```

Or use modular includes:

```cpp
#include "NeuHttpServer.h"
#include "NeuHttpClient.h"
```

## Arduino Library Manager (Recommended)

1. In the Arduino IDE, go to **Sketch > Include Library > Manage Libraries...**
2. Search for **NeuWiFi**
3. Click **Install**

## PlatformIO

```ini
lib_deps = ulywae/NeuWiFi
```

## Manual Installation (Arduino)

1. Download the repository as `.zip`
2. Open Arduino IDE
3. Go to **Sketch > Include Library > Add .ZIP Library**
4. Select the downloaded file

---

# Quick Start

## Basic STA Connection

```cpp
#include <NeuWiFi.h>

void setup() {
    Serial.begin(115200);

    NeuWiFi.begin();
    NeuWiFi.startSTA("MY_HOME_SSID", "MY_PASSWORD");

    if (NeuWiFi.isConnected()) {
        Serial.printf("Connected! IP: %s\n", NeuWiFi.getSTAIP());
    }
}

void loop() {}
```

---

## SoftAP Example

```cpp
#include <NeuWiFi.h>

void setup()
{
    NeuWiFi.begin();
    NeuWiFi.startAP("Neu-AP", "12345678");
}
```

---

## Hybrid AP + STA Example

```cpp
#include <NeuWiFi.h>

void setup()
{
    NeuWiFi.begin();

    NeuWiFi.startHybrid(
        "HomeWiFi",
        "12345678",
        "NeuHybrid",
        "87654321"
    );
}
```

---

# HTTP Server Example

```cpp
#include "NeuHttpServer.h"

NeuHttpServer server;

void setup()
{
    server.begin(80);

    server.on("/", "Hello from NeuHttpServer");
}
```

---

# WebSocket Example

```cpp
#include "NeuHttpServer.h"

NeuHttpServer server;

void onWS(int id, NeuWSEvent event, const char *payload, size_t len)
{
    if(event == WS_EVENT_TEXT)
    {
        server.wsSendText(id, "pong");
    }
}

void setup()
{
    server.begin();
    server.onWS("/ws", onWS);
}
```

---

# UDP Example

```cpp
#define USE_NEU_UDP
#include <NeuWiFi.h>

NeuUDP udp;

void onPacket(const uint8_t *data, size_t len, const char *ip, uint16_t port, void *user) {
    Serial.printf("Received %d bytes from %s:%d\n", len, ip, port);
}

void setup() {
    NeuWiFi.begin();
    NeuWiFi.startSTA("MY_HOME_SSID", "MY_PASSWORD");

    udp.begin(5000);
    udp.onPacket(onPacket);
}

void loop() {}
```

---

# HTTP Client Example

```cpp
#define USE_NEU_HTTP_CLIENT
#include <NeuWiFi.h>

char response[512];

void setup()
{
    NeuHttpClient::get(
        "https://example.com",
        nullptr,
        0,
        response,
        sizeof(response)
    );

    printf("%s\n", response);
}
```

---

# Access Point + DNS Captive Portal

```cpp
#define USE_NEU_DNS_SERVER
#define USE_NEU_HTTP_SERVER
#include <NeuWiFi.h>

NeuHttpServer server;
NeuDNSServer dnsServer;

void setup() {
    NeuWiFi.begin();
    NeuWiFi.startAP("NeuWiFi-Setup", "12345678");

    // Start Web Server
    server.begin(80);
    server.on("/", "<h1>Welcome! Device Ready.</h1>");

    // Start DNS Server (Redirect all domains to AP IP)
    dnsServer.begin();
}

void loop() {}
```

---

# DNS Server Example

```cpp
#include "NeuHttpServer.h"

NeuDnsServer dns;

void setup()
{
    dns.start();
}
```

---

# mDNS Example

```cpp
#define USE_NEU_MDNS
#include <NeuWiFi.h>

void setup()
{
    Serial.begin(115200);

    NeuWiFi.begin();
    NeuWiFi.startAP("NeuWiFi-Setup", "12345678");

    NeuMDNS.addService("http", "tcp", 80);
}
```

---

# API Reference

# NeuWiFi API

## Lifecycle

| Function  | Description             |
| --------- | ----------------------- |
| `init()`  | Initialize WiFi runtime |
| `begin()` | Alias for init()        |
| `stop()`  | Stop WiFi runtime       |

---

## STA / AP Operations

| Function        | Description          |
| --------------- | -------------------- |
| `startSTA()`    | Start STA mode       |
| `startAP()`     | Start SoftAP mode    |
| `startHybrid()` | Start AP + STA mode  |
| `isConnected()` | Check STA connection |

---

## Configuration

| Function              | Description              |
| --------------------- | ------------------------ |
| `setStaticSTA()`      | Configure static STA IP  |
| `setStaticAP()`       | Configure static AP IP   |
| `setHostname()`       | Set device hostname      |
| `setAutoReconnect()`  | Enable auto reconnect    |
| `setMaxConnection()`  | Set AP client limit      |
| `setChannelAP()`      | Set AP WiFi channel      |
| `setTxPower()`        | Set WiFi TX power        |
| `setMinSecuritySTA()` | Set minimum STA security |

---

## Scan API

| Function           | Description           |
| ------------------ | --------------------- |
| `startScan()`      | Start WiFi scan       |
| `isScanDone()`     | Check scan completion |
| `getScanResults()` | Retrieve scan results |

---

## Status API

| Function              | Description                 |
| --------------------- | --------------------------- |
| `getSTAIP()`          | Get STA IP string           |
| `getAPIP()`           | Get AP IP string            |
| `getSTAIPRaw()`       | Get raw STA IP              |
| `getSTAStatus()`      | Get detailed STA status     |
| `getAPStatus()`       | Get detailed AP status      |
| `getChannel()`        | Get current channel         |
| `getTxPower()`        | Get current TX power        |
| `getModeString()`     | Get WiFi mode string        |
| `getAuthModeString()` | Convert auth mode to string |

---

## Event API

| Function           | Description                  |
| ------------------ | ---------------------------- |
| `setHandler()`     | Register WiFi event callback |
| `getEventString()` | Convert event ID to string   |

---

# NeuHttpServer API

## Server Lifecycle

| Function  | Description       |
| --------- | ----------------- |
| `init()`  | Start HTTP server |
| `begin()` | Alias for init()  |
| `stop()`  | Stop HTTP server  |

---

## HTTP Routes

| Function                   | Description                   |
| -------------------------- | ----------------------------- |
| `on(uri, content)`         | Register static GET route     |
| `on(uri, method, handler)` | Register dynamic route        |
| `onError()`                | Register custom error handler |

---

## WebSocket

| Function        | Description          |
| --------------- | -------------------- |
| `onWS()`        | Register WS endpoint |
| `wsSendText()`  | Send WS text frame   |
| `wsSendBin()`   | Send WS binary frame |
| `wsBroadcast()` | Broadcast WS message |
| `wsSendPing()`  | Send WS ping         |

---

## HTTP Helpers

| Function          | Description                  |
| ----------------- | ---------------------------- |
| `sendText()`      | Send plain text response     |
| `sendJSON()`      | Send JSON response           |
| `redirect()`      | Send HTTP redirect           |
| `getPostParam()`  | Parse POST parameter         |
| `errorToString()` | Convert HTTP error to string |

---

# NeuHttpClient API

| Function | Description               |
| -------- | ------------------------- |
| `get()`  | Execute HTTP GET request  |
| `post()` | Execute HTTP POST request |

---

# NeuUDP API

| Function          | Description              |
| ----------------- | ------------------------ |
| `begin()`         | Start UDP listener       |
| `stop()`          | Stop UDP socket          |
| `send()`          | Send UDP packet          |
| `setBroadcast()`  | Enable broadcast mode    |
| `joinMulticast()` | Join multicast group     |
| `setCallback()`   | Register packet callback |
| `reply()`         | Reply to last sender     |

---

# NeuDnsServer API

| Function      | Description        |
| ------------- | ------------------ |
| `init()`      | Initialize DNS     |
| `begin()`     | Alias for init()   |
| `stop()`      | Stop DNS server    |
| `start()`     | Start DNS task     |
| `isRunning()` | Check status       |
| `onQuery()`   | Set query callback |

---

# NeuMDNS API

| Function / Method                                                  | Description                                                                          |
| ------------------------------------------------------------------ | ------------------------------------------------------------------------------------ |
| `NeuMDNSClass()`                                                   | Constructor: Initialize mDNS handler instance                                        |
| `init(hostname, instance_name)`                                    | Initialize mDNS stack, set device hostname and display name                          |
| `begin(hostname, instance_name)`                                   | Alias for init() — start mDNS service                                                |
| `end()`                                                            | Stop mDNS service and free resources                                                 |
| `addService(service_name, proto, port, txt)`                       | Publish a new service to the network (e.g. \_http, \_udp) with optional TXT metadata |
| `startDiscovery(service_name, proto, timeout_ms, interval_ms)`     | Start scanning the network for specified services                                    |
| `stopDiscovery()`                                                  | Stop active discovery scan                                                           |
| `setCallback(callback)`                                            | Register function to receive discovered device events                                |
| `available()`                                                      | Check if any discovered device data is ready to be read                              |
| `readIP()`                                                         | Get IP address of the first device in queue (raw 32-bit value)                       |
| `readIP(out_config)`                                               | Write discovered IP info into IPConfig structure                                     |
| `readIP(out_str)`                                                  | Write discovered IP as human-readable string                                         |
| `readPort()`                                                       | Get port number of the first device in queue                                         |
| `readDevice(out_hostname, max_len, out_ip_str, out_txt)`           | Read full device info: hostname, IP string, and TXT record                           |
| `readDevice(out_hostname, max_len, out_ip_str, out_port, out_txt)` | Read full device info including port number                                          |
| `readDevice(out_hostname, max_len, out_config, out_txt)`           | Read full device info into IPConfig structure                                        |
| `readDevice(out_hostname, max_len, out_config, out_port, out_txt)` | Read full device info into IPConfig + port number                                    |

---

# Configuration

NeuWiFi exposes several compile-time configuration macros.

File:

```cpp
NeuWiFiConfig.h
```

## Capacity Limits

| Macro                      | Default | Description                                           |
| -------------------------- | ------- | ----------------------------------------------------- |
| `NEU_MAX_HTTP_ROUTES`      | 16      | Maximum number of HTTP routes that can be registered. |
| `NEU_MAX_WS_ROUTES`        | 4       | Maximum number of WebSocket endpoints.                |
| `NEU_SERVER_POST_BUF_SIZE` | 512     | Buffer size for HTTP POST payload parsing.            |
| `NEU_SERVER_WS_BUF_SIZE`   | 256     | Buffer size for incoming WebSocket frames.            |
| `NEU_DNS_BUFFER_SIZE`      | 512     | UDP buffer size for DNS captive portal responses.     |

> [!IMPORTANT]
> Always define compile-time macros **before including `NeuWiFi.h`** to ensure correct module compilation and memory allocation.

---

# Runtime Characteristics

## Event Driven

Most modules are event-driven and asynchronous.

Networking operations are handled internally using:

- ESP-IDF event loop
- FreeRTOS tasks
- socket listeners
- callback dispatching
- EventGroup synchronization

---

## Main Loop Independence

NeuWiFi does not depend on the Arduino-style main loop for networking execution.

Even under heavy loop delays:

```cpp
while(true)
{
    delay(10000);
}
```

networking subsystems continue operating independently.

---

## Memory Safety

The library intentionally uses:

- bounded buffers
- cleanup discipline
- guarded allocation
- fallback handling
- fixed runtime limits

This helps reduce runtime instability and hidden fragmentation.

---

## Core Include & Build Flags Usage

NeuWiFi is designed as a **modular ESP32 networking stack**. You can include only what you need and enable features via compile-time flags.

### Basic Include (Recommended)

```cpp
#include <NeuWiFi.h>
```

This single include gives access to all available modules. However, actual features depend on build flags below.

---

### Optional Feature Flags (Compile-time)

Enable modules using `#define` or build flags:

| Macro                 | Module                  | Description                                        |
| --------------------- | ----------------------- | -------------------------------------------------- |
| `USE_NEU_HTTP_SERVER` | HTTP Server + WebSocket | Lightweight web server + persistent connections    |
| `USE_NEU_MDNS`        | mDNS Service Discovery  | Hostname resolution (name.local) + service scanner |
| `USE_NEU_HTTP_CLIENT` | HTTP Client             | GET/POST + HTTPS support                           |
| `USE_NEU_UDP`         | UDP Transport Core      | Enable NeuUDP asynchronous socket layer            |
| `USE_NEU_DNS_SERVER`  | DNS Captive Portal      | Port 53 responder for automatic login portals      |

---

### PlatformIO Example

```ini
build_flags =
    -D USE_NEU_HTTP_SERVER
    -D USE_NEU_MDNS
    -D USE_NEU_DNS_SERVER
```

---

### 🔹 Arduino / ESP-IDF Example

```cpp
#define USE_NEU_HTTP_SERVER
#define USE_NEU_HTTP_CLIENT
#define USE_NEU_MDNS

#include <NeuWiFi.h>
```

---

### 🔹 Design Philosophy

- Modules are **compiled only when needed**
- No forced dependencies between HTTP / UDP / WiFi
- Keep firmware **minimal by default**
- Full stack is optional, not required

---

### Important Note

If no `USE_*` flags are defined:

- Only `WiFi` will be included
- HTTP, WebSocket, UDP, mDNS and DNS modules will be excluded at compile-time

This is intentional to keep memory footprint small for embedded systems.

---

# Notes

This is primarily a personal toolkit built for real-world embedded systems.

If it fits your use case — great.
If not — that’s fine too.

It does exactly what it was designed to do.

---

# Author

**Ulywae (@neufa)**
Part of the **NEU Ecosystem**

---

# License

MIT License

Copyright (c) 2026 Ulywae

---
