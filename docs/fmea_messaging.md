# FMEA: CEELL Messaging Subsystem

## Failure Mode and Effects Analysis

**System**: CEELL Messaging Subsystem (`ceell/network/messaging.c`)
**Scope**: UDP unicast request-response protocol, service dispatch, response correlation
**Date**: 2026-03-04
**Revision**: 1.0

### Scoring Criteria

| Score | Severity (S) | Occurrence (O) | Detection (D) |
|---|---|---|---|
| 1 | No effect | Virtually impossible | Almost certain to detect |
| 2-3 | Minor annoyance | Remote likelihood | High chance of detection |
| 4-5 | Moderate degradation | Occasional | Moderate chance of detection |
| 6-7 | Significant loss of function | Frequent | Low chance of detection |
| 8-9 | Major safety impact | Very frequent | Very low chance of detection |
| 10 | Catastrophic / hazardous | Inevitable | Impossible to detect |

**RPN** = Severity x Occurrence x Detection (range 1-1000, action threshold >= 100)

---

### Failure Mode Analysis

| # | Failure Mode | Potential Cause | Local Effect | System Effect | S | O | D | RPN | Recommended Mitigation |
|---|---|---|---|---|---|---|---|---|---|
| FM-MSG-001 | **Response slot busy (single pending request)** | Concurrent `ceell_msg_send()` calls from multiple threads or the lifecycle loop issuing requests while another is in flight. | Second request overwrites `pending_seq`, causing the first caller to never receive its response (semaphore never signaled). | First request times out; caller sees `-ETIMEDOUT` despite a valid response arriving. Second request may succeed or also timeout depending on timing. | 5 | 4 | 4 | 80 | Implement a request table keyed by sequence number instead of a single pending slot, or enforce single-caller serialization with a request mutex. Log when response slot collision is detected. |
| FM-MSG-002 | **Receive socket failure** | Socket creation fails during `ceell_messaging_init()` due to resource exhaustion (file descriptor limit, network stack not ready). | `ceell_messaging_init()` returns negative error code. RX thread is never started; no incoming messages are processed. | Node cannot respond to any service requests. Outgoing requests may still be sent but no responses can be received. | 7 | 2 | 2 | 28 | Error is already logged and returned. Main boot sequence logs the failure. Future enhancement: retry initialization after delay, or expose a health status flag that discovery can broadcast. |
| FM-MSG-003 | **Send socket failure (ceell_sendto returns error)** | Network interface down, ARP resolution failure (known issue in Renode EMAC), buffer exhaustion in network stack. | `ceell_msg_send()` returns the negative error code from `ceell_sendto`. Request is lost. | Caller does not reach the remote service. If the caller retries (like the echo test in lifecycle), the request may eventually succeed when the network recovers. | 5 | 3 | 3 | 45 | Callers should implement retry with backoff. The lifecycle echo test already retries on failure. Consider adding a send-retry count as a Kconfig option for production. |
| FM-MSG-004 | **Response timeout (deadline miss)** | Remote peer is slow to respond, network congestion drops or delays the UDP packet, remote service handler blocks for too long. | `ceell_sem_take()` returns after the configured timeout. Caller receives `-ETIMEDOUT`. | Service request is lost from the caller's perspective. If the remote peer eventually sends a response, it arrives after the caller has moved on and is silently discarded (sequence number no longer matches `pending_seq`). | 5 | 4 | 3 | 60 | Timeout is already configurable per-call via the `timeout` parameter. Consider adding deadline monitoring: log when responses arrive after timeout for diagnostics. Implement request-level retry in higher-level service callers. |
| FM-MSG-005 | **Message corruption (JSON parse failure)** | Bit errors in UDP payload (no checksum enforcement at application layer), partial packet delivery, buffer truncation if message exceeds `MSG_BUF_SIZE`. | `json_obj_parse()` returns negative error code. The message is silently dropped in `msg_rx_thread_fn()`. | If a request was corrupted: remote peer never responds, causing a timeout on the sender. If a response was corrupted: sender times out waiting for a response that was actually sent. | 4 | 2 | 5 | 40 | UDP has its own checksum (enabled by default in Zephyr). Application-layer mitigation: add an optional CRC or HMAC field in the wire protocol for safety-critical deployments. Log parse failures with the raw buffer content for debugging. |
| FM-MSG-006 | **Peer unreachable (service resolution failure)** | Target peer has expired from the discovery table, network partition, peer crashed and has not re-announced. | `ceell_service_find_peer()` returns `-ENOENT`. `ceell_msg_send()` returns `-ENOENT` to the caller. | Service request cannot be routed. If the peer is permanently gone, all requests to that service fail until a new peer offering the service is discovered. | 6 | 3 | 2 | 36 | Already handled: `ceell_msg_send()` returns `-ENOENT` with a descriptive log message. Higher-level callers should handle this gracefully (e.g., retry after discovery interval). Future enhancement: return the list of all peers offering a service for load balancing / failover. |
| FM-MSG-007 | **Configuration error (invalid MSG_PORT)** | Kconfig value for `CONFIG_CEELL_MSG_PORT` set to a port already in use or outside valid range (0-65535). `ceell_bind()` fails with `-EADDRINUSE` or `-EINVAL`. | `ceell_messaging_init()` returns error. Messaging subsystem is non-functional. | All service requests and responses fail. Node can still discover peers but cannot interact with them. | 7 | 1 | 2 | 14 | Port conflict is detected at bind time and logged. Kconfig should validate the range (already constrained to `int`). Documentation should specify that port 5556 is reserved for CEELL messaging. CI could include a config validation step. |
| FM-MSG-008 | **Thread starvation (RX thread starved by higher-priority threads)** | RX thread runs at priority 7. If discovery, OTA, or application threads run at equal or higher priority and consume significant CPU, the RX thread may not get scheduled promptly. | Incoming messages queue in the socket receive buffer. If the buffer fills, subsequent messages are dropped by the network stack. | Responses arrive late (causing sender timeouts) or are lost entirely. Requests from remote peers go unanswered, causing their senders to timeout. | 6 | 2 | 5 | 60 | RX thread priority (7) is already equal to discovery threads. OTA runs at priority 8 (lower). Ensure no application thread runs at priority <= 7 without justification. Monitor socket buffer overflows via network stack statistics. Consider increasing `CONFIG_NET_BUF_RX_COUNT` for production. |
| FM-MSG-009 | **Buffer overflow in response payload** | Service handler writes more than 128 bytes to `rsp_payload`, or incoming response `payload` field exceeds `struct ceell_msg.payload` (128 bytes). | `strncpy` truncates the payload silently. Caller receives a truncated response. | Partial data returned to the service consumer. For structured data (JSON sub-payloads), truncation may cause parse errors in the consumer. | 4 | 3 | 4 | 48 | Payload buffer sizes are fixed at 128 bytes. Service handlers receive `rsp_len` to enforce bounds. Consider making the payload size configurable via Kconfig for deployments that need larger payloads. Add a `truncated` flag to the response for explicit signaling. |
| FM-MSG-010 | **Socket resource leak on repeated init** | If `ceell_messaging_init()` is called multiple times (programming error), a new socket is created each time without closing the previous one. | File descriptor leak. Eventually socket creation fails with `-EMFILE`. | After several leaked sockets, the entire network stack may become unable to allocate new sockets, affecting discovery and OTA as well. | 7 | 1 | 3 | 21 | Guard `ceell_messaging_init()` with a static `initialized` flag or check if `msg_sock >= 0` before creating a new socket. Close the old socket before re-initializing if re-init is a supported use case. |

---

### Risk Priority Summary

| RPN Range | Count | Action |
|---|---|---|
| 1-49 | 6 | Accept risk, monitor in future reviews |
| 50-99 | 3 | Implement recommended mitigations in next development phase |
| 100+ | 0 | No immediate critical actions required |

### Top 3 Risks by RPN

1. **FM-MSG-001** (RPN 80) — Response slot collision. Highest risk due to the single-pending-request design. Mitigate by adding a request correlation table or caller serialization.
2. **FM-MSG-004** (RPN 60) — Response timeout. Moderate risk, inherent in UDP-based protocols. Mitigate with per-caller retry logic and deadline monitoring.
3. **FM-MSG-008** (RPN 60) — RX thread starvation. Moderate risk in loaded systems. Mitigate with priority analysis and socket buffer tuning.

---

### Action Items

| Priority | Action | Target Phase | Owner |
|---|---|---|---|
| 1 | Replace single pending response slot with sequence-keyed correlation table | Phase 7 (Priority Messaging) | TBD |
| 2 | Add deadline monitoring: log late responses for diagnostics | Phase 7 | TBD |
| 3 | Add `initialized` guard to `ceell_messaging_init()` | Phase 6 or 7 | TBD |
| 4 | Document thread priority map for all CEELL threads | Phase 6 | TBD |
| 5 | Add optional CRC/HMAC field for safety-critical deployments | Future | TBD |

---

### Revision History

| Rev | Date | Author | Changes |
|---|---|---|---|
| 1.0 | 2026-03-04 | CEELL Team | Initial FMEA for messaging subsystem |
