# CEELL Requirements Specification

This document defines the verifiable requirements for the CEELL universal
configurable ECU node middleware. Each requirement has a unique identifier
that can be referenced in source code (`/* REQ-xxx-yyy */` comments) and
test cases to establish full traceability.

---

## REQ-OSAL: OS Abstraction Layer

| ID | Title | Description |
|---|---|---|
| REQ-OSAL-001 | OSAL Isolation | All CEELL middleware code outside `ceell/osal/` SHALL access RTOS primitives exclusively through the OSAL API. No direct `#include <zephyr/...>` headers are permitted in `ceell/core/`, `ceell/network/`, `ceell/ota/`, or `src/`. |
| REQ-OSAL-002 | Thread Abstraction | The OSAL SHALL provide a platform-independent thread creation, abort, priority-set, and naming API (`ceell_thread_create`, `ceell_thread_abort`, `ceell_thread_priority_set`, `ceell_thread_name_set`). |
| REQ-OSAL-003 | Synchronization Abstraction | The OSAL SHALL provide mutex (`ceell_mutex_*`), semaphore (`ceell_sem_*`), and message queue (`ceell_msgq_*`) primitives with timeout support. |
| REQ-OSAL-004 | Time Abstraction | The OSAL SHALL provide monotonic uptime (`ceell_uptime_get`), sleep (`ceell_sleep`), and hardware cycle counter (`ceell_cycle_get`) with platform-independent timeout types. |
| REQ-OSAL-005 | Socket Abstraction | The OSAL SHALL provide BSD-style socket operations (`ceell_socket`, `ceell_bind`, `ceell_sendto`, `ceell_recvfrom`, `ceell_setsockopt`, `ceell_inet_pton`, `ceell_connect`, `ceell_close`) wrapping the platform socket API. |

---

## REQ-MSG: Messaging Subsystem

| ID | Title | Description |
|---|---|---|
| REQ-MSG-001 | Request-Response Protocol | The messaging subsystem SHALL implement a JSON-based request/response protocol over UDP unicast, with sequence numbers for correlation and configurable timeout. |
| REQ-MSG-002 | Service Dispatch | Incoming requests SHALL be dispatched to the registered service handler based on the `to_svc` field. Unknown services SHALL receive an error response with `-ENOENT`. |
| REQ-MSG-003 | Sequence Number Tracking | Each outgoing request SHALL carry a monotonically increasing sequence number. Responses SHALL be matched to pending requests by sequence number. |
| REQ-MSG-004 | Messaging Initialization | The messaging subsystem SHALL bind a UDP socket on `CONFIG_CEELL_MSG_PORT` and start an RX thread to process incoming messages. |
| REQ-MSG-005 | Wire Protocol Format | Messages SHALL be encoded as JSON objects with fields: `type` ("req" or "rsp"), `seq`, `from_id`, and either `to_svc`+`payload` (request) or `status`+`payload` (response). |

---

## REQ-SAF: Safety and Reliability

| ID | Title | Description |
|---|---|---|
| REQ-SAF-001 | Graceful Degradation | Failure of any single subsystem (discovery, messaging, OTA) SHALL NOT prevent the node from continuing to operate in its primary role. Initialization failures SHALL be logged but not halt the system unless fatal. |
| REQ-SAF-002 | Health Monitoring | The lifecycle loop SHALL periodically log health status including node ID and active peer count, enabling external monitoring systems to detect anomalies. |
| REQ-SAF-003 | Peer Expiry | Peers not seen within `CONFIG_CEELL_PEER_TIMEOUT_MS` SHALL be removed from the peer table to prevent stale routing. |
| REQ-SAF-004 | Thread Isolation | Each subsystem (discovery TX, discovery RX, messaging RX, OTA) SHALL run in a dedicated thread with independent stack allocation to prevent stack corruption across modules. |
| REQ-SAF-005 | Mutex Protection | All shared data structures (peer table, service registry, response slot, OTA state) SHALL be protected by mutexes or atomic operations to prevent data races. |

---

## REQ-NET: Network Discovery and Service Resolution

| ID | Title | Description |
|---|---|---|
| REQ-NET-001 | Multicast Discovery | Nodes SHALL announce their identity (node_id, role, name, IP, services) via UDP multicast to `CONFIG_CEELL_DISCOVERY_GROUP`:`CONFIG_CEELL_DISCOVERY_PORT` at `CONFIG_CEELL_DISCOVERY_INTERVAL_MS` intervals. |
| REQ-NET-002 | Service Resolution | The system SHALL resolve a service name to a peer IP address by scanning the discovery peer table for peers whose services CSV contains the requested name (exact token match, not substring). |
| REQ-NET-003 | Peer Table Management | The discovery subsystem SHALL maintain a bounded peer table (`CONFIG_CEELL_DISCOVERY_MAX_PEERS`), updating existing peers on re-announcement and adding new peers when capacity allows. |

---

## REQ-OTA: Over-the-Air Firmware Updates

| ID | Title | Description |
|---|---|---|
| REQ-OTA-001 | OTA State Machine | The OTA client SHALL transition through states IDLE -> CHECKING -> DOWNLOADING -> VALIDATING -> MARKING -> REBOOTING, with ERROR reachable from any active state. State transitions SHALL be atomic. |
| REQ-OTA-002 | Version Comparison | The OTA client SHALL parse semantic version strings ("major.minor.patch"), compare the server manifest version against the running firmware version, and only proceed with download if the remote version is strictly greater. |
| REQ-OTA-003 | Firmware Download and Swap | The OTA client SHALL download firmware via HTTP to the secondary flash slot using stream-flash writes, confirm the current image, mark the new image for MCUboot test swap, and reboot. Size mismatches SHALL be detected and reported. |

---

## Versioning

- Document version: 1.0
- Applicable firmware version: 0.2.x
- Last updated: 2026-03-04
