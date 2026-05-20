/**
 * @file NeuWiFi.h
 * @author Ulywae (@Neufa)
 * @brief Professional, Thread-Safe ESP32 Networking Framework for ESP-IDF & Arduino.
 * @version 1.1.0
 * @date 2026
 *
 * @details
 * NeuWiFi is a modular, high-velocity networking framework built exclusively
 * to harness the native hardware capabilities of the ESP32 SoC family. By avoiding
 * heavy cross-platform abstraction layers, it provides ultra-lean memory footprints
 * and native execution speeds, optimized specifically for real-time embedded applications.
 *
 * This framework is engineered around a centralized state-management architecture,
 * ensuring all services operate in perfect synchronization. It features automatic
 * configuration handling, endianness correction for network compatibility, and
 * robust event-driven operations.
 *
 * Core Features:
 * - Thread-safe, event-driven Wi-Fi Station (STA) link orchestration with auto-recovery
 * - Autonomous Access Point (AP) local beacon broadcasting & client tracking
 * - Concurrent Hybrid (AP+STA) dual-mode operations with automatic mode prioritization
 * - Granular Static IPv4 configuration mapping with validation and LwIP byte-order correction
 * - Non-blocking background network topology scanning and analysis
 * - Low-overhead socket routing for optimized UDP packet streams
 * - High-efficiency Asynchronous HTTP & WebSocket server engines with zero-copy principles
 * - Automated Port 53 DNS interception & intelligent client filtering for Captive Portals
 * - Memory-optimized, non-blocking outbound HTTP/HTTPS client state engine
 * - Conflict-aware, native Multicast DNS (NeuMDNS) responder interface
 * - Centralized State Manager (`SharedState`) for unified system awareness and event propagation
 *
 * Architectural Goals:
 * - Ultra-lean runtime with minimal dynamic heap footprint; static memory preference
 * - Deterministic, non-blocking asynchronous execution workflows
 * - Embedded-safe networking with zero-copy data routing principles
 * - Modular compilation system to strictly exclude unused assets from compiled binary
 * - Streamlined, beginner-friendly API wrapping complex lower-level OS handles
 * - Production-grade reliability designed for industrial-scale IoT applications
 * - Full separation of concerns: Configuration, State, Logic, and Transport layers
 *
 * Optional Compile-Time Module Switches:
 * - USE_NEU_UDP              Enables low-overhead UDP runtime routines & multicast support
 * - USE_NEU_HTTP_SERVER      Enables the asynchronous embedded web server engine
 * - USE_NEU_WEBSOCKET_SERVER Enables high-velocity full-duplex WebSocket server streams
 * - USE_NEU_WEBSOCKET        Enables persistent external WebSocket client uplinks
 * - USE_NEU_DNS_SERVER       Enables local DNS resolving with smart client tracking for captive portals
 * - USE_NEU_HTTP_CLIENT      Enables non-blocking outbound HTTP/HTTPS networking
 * - USE_NEU_MDNS             Enables native, conflict-aware local mDNS resolution
 *
 * Framework Logical Topology:
 * NeuWiFi
 * ├── Core Foundation
 * │   ├── IPConfig             → Safe, validated network address storage & conversion
 * │   └── SharedState          → Global State Manager & Event Bus (System "Brain")
 * ├── Wi-Fi Link Core
 * │   ├── STA Management       → Connection, DHCP/Static IP, Reconnection logic
 * │   ├── AP Management        → Beaconing, IP Allocation, Client Tracking
 * │   └── Event System         → Mode changes, link status propagation
 * ├── UDP Socket Runtime
 * │   ├── Transport Layer       → Raw send/receive, buffer management
 * │   └── Protocol Handlers    → Broadcast, Multicast, Unicast routing
 * ├── Web Engine
 * │   ├── HTTP Server          → Async request/response routing
 * │   ├── WebSocket Server     → Full-duplex persistent connections
 * │   └── File System Router    → Static content serving
 * ├── DNS Resolver Layer
 * │   ├── Packet Parser/Builder → DNS protocol handling
 * │   ├── Client Tracker        → "New User" vs "Authenticated User" filtering
 * │   └── Redirect Logic        → Intelligent captive portal triggering
 * ├── Outbound Networking
 * │   └── HTTP / HTTPS Client   → Non-blocking external API consumption
 * └── Local Name Resolver
 *     └── NeuMDNS               → Service discovery, hostname resolution
 */

#ifndef NEU_WIFI_H
#define NEU_WIFI_H

// =============================================================================
// PLATFORM & HARDWARE VALIDATION
// =============================================================================
#if !defined(ESP32)
/**
 * @brief Cross-Platform Compiler Protection.
 *
 * The Neu ecosystem architecture focuses on single-chip optimization.
 * NeuWiFi is exclusively tailored and optimized for the ESP32 silicon family
 * (fully supporting both Arduino ESP32 core and native ESP-IDF runtimes).
 * No legacy or alternative architecture support is provided to ensure maximum efficiency.
 */
#error "Platform not supported! NeuWiFi currently only supports the ESP32 architecture."
#endif

// =============================================================================
// BASE SYSTEM DEPENDENCIES
// =============================================================================
/**
 * @note Core utility and state management modules are always included.
 * These form the foundation of the architecture and are required by all components.
 */
#include "SystemState/SharedState.h"
#include "IPConfig.h"

// =============================================================================
// CORE NETWORKING MODULES
// =============================================================================
/**
 * @brief Main Wi-Fi Management Engine (NeuWiFi Core).
 *
 * Handles the physical radio control, mode switching logic, IP configuration
 * application, and synchronization with SharedState. This is the primary control
 * interface used by the application layer.
 */
#include "wifi/_NeuWiFi.h"

#if defined(USE_NEU_UDP)
/**
 * @brief Lightweight UDP Runtime Interface (NeuUDP Engine).
 *
 * Provides high-performance socket abstraction. Automatically starts/stops
 * based on network status changes broadcasted by SharedState. Supports custom
 * parsing logic and direct memory access for high-speed data transmission.
 */
#include "udp/_NeuUDP.h"
#endif

// =============================================================================
// HTTP & WEBSOCKET SERVER MODULES (OPTIONAL COMPONENT LAYERS)
// =============================================================================
#if defined(USE_NEU_HTTP_SERVER) ||      \
    defined(USE_NEU_WEBSOCKET_SERVER) || \
    defined(USE_NEU_WEBSOCKET)
/**
 * @brief High-efficiency Asynchronous HTTP and WebSocket server execution engine.
 *
 * Built on native queues and callbacks. Integrates tightly with the DNS and
 * client tracking systems to serve content only when necessary.
 */
#include "http/_NeuHttpServer.h"
#endif

#if defined(USE_NEU_DNS_SERVER)
/**
 * @brief Lightweight Local DNS Resolver for Network Captive Portals.
 *
 * Operates on port 53. Uses `SharedState` and client tracker logic to decide
 * whether to resolve normally or redirect all domains to the local AP IP.
 * Essential for seamless user onboarding.
 */
#include "dns/_NeuDnsServer.h"
#endif

// =============================================================================
// HTTP CLIENT MODULE (OPTIONAL COMPONENT LAYER)
// =============================================================================
#if defined(USE_NEU_HTTP_CLIENT)
/**
 * @brief Non-blocking, ultra-lean HTTP/HTTPS client state engine.
 *
 * Designed for outbound API calls. Does not block the main loop; operates via
 * state machines and callbacks. Handles secure connections (TLS) with minimal overhead.
 */
#include "http/_NeuHttpClient.h"
#endif

// =============================================================================
// MULTICAST DNS RUNTIME (OPTIONAL COMPONENT LAYER)
// =============================================================================
#if defined(USE_NEU_MDNS)
/**
 * @brief Next-Generation Local Domain Resolver (NeuMDNS Interface).
 *
 * Native implementation that avoids system conflicts. Advertises services
 * automatically when the relevant interface (STA/AP) becomes ready.
 * Allows devices to be discovered via names like `mydevice.local`.
 */
#include "mdns/_NeuMDNS.h"
#endif

#endif // NEU_WIFI_H