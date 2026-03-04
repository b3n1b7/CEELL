# CEELL Timing Budgets

## Overview

This document defines the target timing budgets for critical code paths in the
CEELL middleware. Each budget represents the maximum acceptable execution time
for a specific operation. Violations are tracked by the timing budget framework
(`ceell/core/timing_budget.h`) and three or more consecutive violations would
trigger safe-state escalation in production.

## Target Timing Values

| Code Path                   | Label                | Budget   | Rationale                                               |
|-----------------------------|----------------------|----------|---------------------------------------------------------|
| Critical end-to-end         | `critical_e2e`       | < 5 ms   | Brake command to actuator response; safety-critical      |
| Message dispatch             | `msg_dispatch`       | < 100 us | Socket read to priority queue insertion                  |
| Priority handler             | `priority_handler`   | < 200 us | Dequeue to service handler invocation                    |
| Discovery announcement       | `discovery_announce` | < 1 ms   | Compose and send multicast discovery message             |
| Lifecycle tick               | `lifecycle_tick`     | < 10 ms  | One iteration of the main lifecycle loop                 |

## Budget Definitions in Code

These budgets should be registered during system initialization:

```c
ceell_timing_budget_define("critical_e2e",       5000);   /* 5 ms */
ceell_timing_budget_define("msg_dispatch",        100);   /* 100 us */
ceell_timing_budget_define("priority_handler",    200);   /* 200 us */
ceell_timing_budget_define("discovery_announce", 1000);   /* 1 ms */
ceell_timing_budget_define("lifecycle_tick",    10000);   /* 10 ms */
```

## Measurement Points

### Critical End-to-End (< 5 ms)

Measured from the moment a critical message is received on the network to the
moment the corresponding service handler completes execution.

```c
ceell_timing_start("critical_e2e", &handle);
/* ... receive message, dispatch, execute handler ... */
ceell_timing_stop("critical_e2e", handle);
```

### Message Dispatch (< 100 us)

Measured from socket `recvfrom()` return to successful `msgq_put()` into the
priority queue.

```c
ceell_timing_start("msg_dispatch", &handle);
len = ceell_recvfrom(sock, buf, sizeof(buf), 0, &src, &srclen);
/* ... parse, validate, enqueue ... */
ceell_msgq_put(&queue, &msg, CEELL_NO_WAIT);
ceell_timing_stop("msg_dispatch", handle);
```

### Priority Handler (< 200 us)

Measured from priority queue `msgq_get()` to service handler return.

```c
ceell_timing_start("priority_handler", &handle);
ceell_msgq_get(&queue, &msg, CEELL_FOREVER);
handler = find_handler(msg.service);
handler(&msg);
ceell_timing_stop("priority_handler", handle);
```

### Discovery Announcement (< 1 ms)

Measured from the start of discovery message composition to `sendto()` return.

```c
ceell_timing_start("discovery_announce", &handle);
/* ... build JSON, sendto multicast ... */
ceell_timing_stop("discovery_announce", handle);
```

### Lifecycle Tick (< 10 ms)

Measured across one full iteration of the lifecycle loop, including peer
expiry checks, health monitoring, and watchdog feeding.

```c
ceell_timing_start("lifecycle_tick", &handle);
/* ... peer_expire, health_check, watchdog_feed ... */
ceell_timing_stop("lifecycle_tick", handle);
```

## Violation Handling

The timing budget framework tracks total and consecutive violation counts:

- **Total violations**: Logged for diagnostics and trend analysis.
- **Consecutive violations (3+)**: Trigger a warning log that would escalate
  to safe state in production (Phase 7 integration). This prevents transient
  spikes from causing unnecessary shutdowns while catching sustained
  performance degradation.

## Hardware Considerations

- **Native Sim**: `CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC=1000000` (1 MHz).
  Timing resolution is 1 us. Good for functional testing but not
  representative of real hardware timing.

- **S32K344 (MR-CANHUBK3)**: Cortex-M7 at 160 MHz. Hardware cycle counter
  gives ~6 ns resolution. All budgets are achievable with typical code paths.

- **S32K5xx (Future)**: Cortex-M7 at up to 320 MHz with TSN ENETC.
  Network-induced latency is bounded by TAS gate scheduling.

## Reporting

Call `ceell_timing_budget_report()` to print all budgets and their violation
counts. This can be triggered via the messaging service interface or during
periodic health checks.

```
=== Timing Budget Report ===
  critical_e2e          budget=5000 us  violations=0
  msg_dispatch          budget=100 us   violations=2
  priority_handler      budget=200 us   violations=0
  discovery_announce    budget=1000 us  violations=0
  lifecycle_tick        budget=10000 us violations=1
============================
```
