# CEELL Requirements Traceability Matrix

This document maps each requirement to its implementing source files and
verifying test cases. The `/* REQ-xxx-yyy */` comment markers in source
and test files enable automated traceability checking via
`tools/check_traceability.py`.

---

## OSAL Requirements

| Requirement | Source Implementation | Test Verification |
|---|---|---|
| REQ-OSAL-001 | `ceell/osal/osal.h` (enforced by `tools/check_osal_isolation.sh`) | CI static-analysis job |
| REQ-OSAL-002 | `ceell/osal/osal_thread.h`, `ceell/osal/zephyr/osal_thread.c` | `tests/unit/osal/src/main.c` (osal_thread suite) |
| REQ-OSAL-003 | `ceell/osal/osal_sync.h`, `ceell/osal/zephyr/osal_sync.c` | `tests/unit/osal/src/main.c` (osal_mutex, osal_sem, osal_msgq suites) |
| REQ-OSAL-004 | `ceell/osal/osal_time.h`, `ceell/osal/zephyr/osal_time.c` | `tests/unit/osal/src/main.c` (osal_time suite) |
| REQ-OSAL-005 | `ceell/osal/osal_socket.h`, `ceell/osal/zephyr/osal_socket.c` | `tests/robot/test_discovery.robot` (socket used for multicast), `tests/robot/test_services.robot` (socket used for messaging) |

## Messaging Requirements

| Requirement | Source Implementation | Test Verification |
|---|---|---|
| REQ-MSG-001 | `ceell/network/messaging.c` (`ceell_msg_send`, `msg_rx_thread_fn`) | `tests/robot/test_services.robot` (echo request-response) |
| REQ-MSG-002 | `ceell/network/messaging.c` (`handle_request`) | `tests/robot/test_services.robot` (service dispatch validated via echo) |
| REQ-MSG-003 | `ceell/network/messaging.c` (`next_seq`, `pending_seq`, `handle_response`) | `tests/robot/test_services.robot` (implicit via successful echo round-trip) |
| REQ-MSG-004 | `ceell/network/messaging.c` (`ceell_messaging_init`) | `tests/robot/test_boot.robot` (messaging initialization logged) |
| REQ-MSG-005 | `ceell/network/messaging.c` (`msg_req_descr`, `msg_rsp_descr`) | `tests/robot/test_services.robot` (JSON wire format validated end-to-end) |

## Safety Requirements

| Requirement | Source Implementation | Test Verification |
|---|---|---|
| REQ-SAF-001 | `src/main.c` (non-fatal error handling for discovery, messaging, OTA) | `tests/robot/test_boot.robot` (verifies boot completes) |
| REQ-SAF-002 | `ceell/core/lifecycle.c` (`ceell_lifecycle_run` — CEELL_HEALTH log) | `tests/robot/test_boot.robot` (CEELL_HEALTH line matched), `tests/robot/test_discovery.robot` (peers= count verified) |
| REQ-SAF-003 | `ceell/network/discovery.c` (`ceell_discovery_expire_peers`) | `tests/robot/test_discovery.robot` (peer expiry tested implicitly via lifecycle) |
| REQ-SAF-004 | `ceell/network/discovery.c` (TX/RX threads), `ceell/network/messaging.c` (RX thread), `ceell/ota/ota_client.c` (OTA thread) | `tests/robot/test_boot.robot` (threads created at boot) |
| REQ-SAF-005 | `ceell/network/discovery.c` (`peer_mutex`), `ceell/core/service_registry.c` (`svc_mutex`), `ceell/network/messaging.c` (`rsp_mutex`), `ceell/ota/ota_state.c` (atomic state) | `tests/unit/osal/src/main.c` (mutex/sem/atomic tests), `tests/unit/ota_state/src/main.c` (atomic state transitions) |

## Network Requirements

| Requirement | Source Implementation | Test Verification |
|---|---|---|
| REQ-NET-001 | `ceell/network/discovery.c` (`tx_thread_fn`, `ceell_discovery_init`) | `tests/robot/test_discovery.robot` (CEELL_DISCOVERED lines matched) |
| REQ-NET-002 | `ceell/network/service_discovery.c` (`ceell_service_find_peer`, `csv_contains`) | `tests/robot/test_services.robot` (echo service resolved to peer IP) |
| REQ-NET-003 | `ceell/network/discovery.c` (`update_peer`, `ceell_discovery_get_peers`) | `tests/robot/test_discovery.robot` (multiple peers discovered and counted) |

## OTA Requirements

| Requirement | Source Implementation | Test Verification |
|---|---|---|
| REQ-OTA-001 | `ceell/ota/ota_state.c` (atomic state get/set), `ceell/ota/ota_client.c` (state transitions in `ota_check_and_update`) | `tests/unit/ota_state/src/main.c` (all state transitions and string representations) |
| REQ-OTA-002 | `ceell/ota/ota_version.c` (`ceell_ota_ver_parse`, `ceell_ota_ver_cmp`) | `tests/unit/ota_version/src/main.c` (parse, compare, format tests) |
| REQ-OTA-003 | `ceell/ota/ota_client.c` (`download_firmware`, `ota_check_and_update`) | Manual integration test (requires OTA server + MCUboot build) |

---

## Coverage Summary

| Category | Total | With Source | With Tests | Gaps |
|---|---|---|---|---|
| OSAL | 5 | 5 | 5 | 0 |
| Messaging | 5 | 5 | 5 | 0 |
| Safety | 5 | 5 | 5 | 0 |
| Network | 3 | 3 | 3 | 0 |
| OTA | 3 | 3 | 3 | 0 |
| **Total** | **21** | **21** | **21** | **0** |

Note: REQ-OTA-003 test coverage is via manual integration testing only
(requires MCUboot sysbuild + OTA server). Automated integration test for
OTA download is planned for a future phase.

---

- Document version: 1.0
- Last updated: 2026-03-04
