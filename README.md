# NeuWiFi

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform: Arduino](https://img.shields.io/badge/Platform-Arduino-00878F?logo=arduino&logoColor=white)](https://arduino.cc)
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-blue?logo=espressif&logoColor=white)](https://espressif.com)
[![Architecture: 32-bit](https://img.shields.io/badge/Architecture-32--bit-orange.svg)]()
[![Design: Deterministic](https://img.shields.io/badge/Design-Deterministic-critical.svg)]()

Professional ESP32 Networking Toolkit for ESP-IDF

**NeuWiFi** is a lightweight, event-driven networking toolkit designed specifically for ESP32 systems.
It focuses on deterministic runtime behavior, bounded memory usage, modular architecture, and practical embedded networking.

Unlike many high-level wrappers, NeuWiFi is intentionally designed around:

- predictable runtime behavior
- low idle overhead
- defensive resource handling
- event-driven architecture
- modular subsystem usage
- real-world embedded deployment

---

# Features

## Core WiFi

- STA (Station) mode
- SoftAP mode
- Hybrid AP+STA mode
- Static IP support
- DNS configuration
- Hostname configuration
- Auto reconnect
- WiFi scan API
- Runtime event callbacks
- TX power control
- Security threshold control

## HTTP Server

- Lightweight HTTP server
- Static route support
- Dynamic route handlers
- JSON helpers
- Redirect helpers
- POST parameter parser
- Custom error handlers

## WebSocket

- Built-in WebSocket support
- Async frame handling
- Broadcast support
- Ping/Pong support
- Binary frame support
- Client FD management

## HTTP Client

- HTTP GET
- HTTP POST
- HTTPS support
- Custom headers
- Buffered response handling
- Internal response streaming

## UDP

- Async UDP listener
- Broadcast support
- Multicast support
- Callback-driven packet handling
- Dedicated FreeRTOS receive task

## DNS Server

- Captive portal DNS server
- Dynamic AP IP response
- Lightweight UDP implementation
- Minimal memory footprint

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
#include "NeuWiFi.h"

NeuWiFi wifi;

void setup()
{
    wifi.begin();

    bool ok = wifi.startSTA("MyWiFi", "12345678");

    if(ok)
    {
        printf("Connected: %s\n", wifi.getSTAIP());
    }
}
```

---

## SoftAP Example

```cpp
#include "NeuWiFi.h"

NeuWiFi wifi;

void setup()
{
    wifi.begin();
    wifi.startAP("Neu-AP", "12345678");
}
```

---

## Hybrid AP + STA Example

```cpp
#include "NeuWiFi.h"

NeuWiFi wifi;

void setup()
{
    wifi.begin();

    wifi.startHybrid(
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
#include "NeuUDP.h"

NeuUDP udp;

void setup()
{
    udp.begin(5000);

    udp.setCallback([](uint8_t *data, size_t len, const char *ip, uint16_t port)
    {
        printf("Packet from %s:%d\n", ip, port);
    });
}
```

---

# HTTP Client Example

```cpp
#include "NeuHttpClient.h"

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

---

# NeuDnsServer API

| Function  | Description     |
| --------- | --------------- |
| `start()` | Start DNS task  |
| `stop()`  | Stop DNS server |

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
>
> These values are optimized for ESP32 RAM stability and FreeRTOS stack safety.

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

### 🔹 Basic Include (Recommended)

```cpp
#include <NeuWiFi.h>
```

This single include gives access to all available modules. However, actual features depend on build flags below.

---

### 🔹 Optional Feature Flags (Compile-time)

Enable modules using `#define` or build flags:

| Macro                  | Module                  | Description                   |
| ---------------------- | ----------------------- | ----------------------------- |
| `USE_HTTP_SERVER`      | HTTP Server + WebSocket | Lightweight HTTP + WS runtime |
| `USE_WEBSOCKET_SERVER` | WebSocket only          | Standalone WS handler         |
| `USE_WEBSOCKET`        | WebSocket alias         | Compatibility flag            |
| `USE_HTTP_CLIENT`      | HTTP Client             | GET/POST + HTTPS support      |

---

### 🔹 PlatformIO Example

```ini
build_flags =
    -D USE_HTTP_SERVER
    -D USE_WEBSOCKET_SERVER
    -D USE_HTTP_CLIENT
```

---

### 🔹 Arduino / ESP-IDF Example

```cpp
#define USE_HTTP_SERVER
#define USE_HTTP_CLIENT
#define USE_WEBSOCKET_SERVER

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

- Only `WiFi + UDP core` will be included
- HTTP, WebSocket, and DNS modules will be excluded at compile-time

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
