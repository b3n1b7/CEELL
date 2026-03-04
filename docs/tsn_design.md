# TSN Architecture for CEELL

## Overview

Time-Sensitive Networking (TSN) is a set of IEEE 802.1 standards that provide
deterministic, low-latency Ethernet communication. CEELL uses TSN to guarantee
that safety-critical messages (brake, steer) meet their end-to-end deadlines
even under heavy network load.

This document describes the TSN architecture, priority mapping, hardware
requirements, and configuration flow for the CEELL middleware.

## Priority Mapping: CEELL to IEEE 802.1Q

CEELL defines three message priority levels. Each maps to an IEEE 802.1Q
Priority Code Point (PCP) value and corresponding traffic class.

| CEELL Priority    | Enum Value | 802.1Q PCP | Traffic Class     | Description                      |
|-------------------|------------|------------|-------------------|----------------------------------|
| CEELL_MSG_CRITICAL | 0          | 7          | Network Control   | Safety-critical: brake, steer    |
| CEELL_MSG_NORMAL   | 1          | 4          | Controlled Load   | Standard service requests         |
| CEELL_MSG_BULK     | 2          | 1          | Background        | Diagnostics, OTA, telemetry      |

### PCP Value Rationale

- **PCP 7 (Network Control)**: Highest priority. Used for brake commands and
  steering inputs that must arrive within 5ms end-to-end. The TSN switch
  forwards these frames with strict priority over all other traffic.

- **PCP 4 (Controlled Load)**: Mid-priority. Service requests and responses
  that have soft real-time requirements (~100ms) but are not safety-critical.

- **PCP 1 (Background)**: Best-effort. Diagnostic dumps, OTA status checks,
  and telemetry can tolerate seconds of delay.

## TSN Building Blocks

### 1. gPTP Time Synchronization (IEEE 802.1AS)

All nodes synchronize their clocks using the generalized Precision Time
Protocol (gPTP). This provides a common time reference across the vehicle
network with sub-microsecond accuracy.

- **Grand Master**: One node (typically the chassis controller) acts as the
  gPTP grand master clock.
- **Slave Nodes**: All other CEELL nodes synchronize to the grand master.
- **Sync Interval**: 125ms (default), configurable per deployment.

### 2. Credit-Based Shaper (CBS) — IEEE 802.1Qav

CBS shapes traffic to prevent burst congestion. Each traffic class is
assigned a bandwidth credit that replenishes over time.

| Traffic Class    | Max Bandwidth | Idle Slope |
|------------------|---------------|------------|
| Network Control  | 25%           | 25 Mbit/s  |
| Controlled Load  | 50%           | 50 Mbit/s  |
| Background       | 25%           | 25 Mbit/s  |

### 3. Time-Aware Shaper (TAS) — IEEE 802.1Qbv

TAS divides each network cycle into time windows (gate slots) during which
only specific traffic classes may transmit.

Example 1ms cycle:

```
|-- 200us CRITICAL --|-- 500us NORMAL --|-- 200us BULK --|-- 100us guard --|
```

- **Guard band**: Prevents frame overlap between time windows.
- **Gate Control List (GCL)**: Programmed into the switch ASIC by the network
  configurator at boot.

## Hardware Requirements

### Current: NXP S32K344 (MR-CANHUBK3)

The S32K344 has a standard Ethernet MAC (EMAC) without TSN hardware support.
CEELL currently runs without TSN — all traffic shares a single best-effort
queue. The TSN API layer is stubbed (`ceell_tsn_available()` returns false).

### Target: NXP S32K5xx Series

The S32K5xx family includes the ENETC (Enhanced Network Controller) with
full TSN support:

- **gPTP hardware timestamping**: IEEE 1588 PTP clock with nanosecond
  resolution
- **8 hardware traffic classes**: Mapped via PCP in VLAN tags
- **CBS and TAS engines**: Programmable via ENETC registers
- **Cut-through switching**: Minimizes store-and-forward latency

Required Zephyr drivers:
- `nxp,s32-enetc` — ENETC Ethernet driver
- `nxp,s32-ptp-clock` — IEEE 1588 PTP clock driver

### ST SR6 P3E (Under Evaluation)

The ST SR6 P3E is being evaluated as an alternative. TSN capabilities
are TBD pending board bring-up.

## Configuration Flow

### Boot Sequence

1. **gPTP Sync** — Node joins the gPTP domain and synchronizes its clock.
   Wait until sync accuracy is within 1us before enabling TAS.

2. **CBS Configuration** — Program idle slopes for each traffic class based
   on the deployment profile (body, chassis, lights).

3. **TAS Gate Schedule** — Load the Gate Control List from flash configuration.
   The GCL defines which queues are open during each time slot of the cycle.

4. **Socket Priority Binding** — `ceell_tsn_set_socket_priority()` sets
   `SO_PRIORITY` on each socket. The ENETC driver maps `SO_PRIORITY` to
   the corresponding hardware queue.

### Runtime

- The CEELL messaging layer calls `ceell_tsn_set_socket_priority()` when
  creating sockets.
- When sending a message, the PCP value is embedded in the VLAN tag by the
  ENETC hardware (no software overhead).
- The timing measurement framework (`ceell/core/timing.h`) monitors actual
  message latencies against budgets defined in the timing budget framework.

## Integration with Timing Budgets

The timing budget framework defines acceptable latency for each code path.
When TSN is active, end-to-end timing violations are correlated with network
statistics to distinguish between software and network-induced delays.

| Code Path                | Budget  | TSN Contribution         |
|--------------------------|---------|--------------------------|
| Critical e2e             | < 5ms   | TAS guarantees < 1ms     |
| Message dispatch         | < 100us | Software only            |
| Discovery announcement   | < 1ms   | CBS-shaped               |

## Future Work

- **Frame Preemption (IEEE 802.1Qbu)**: Allow high-priority frames to
  interrupt low-priority frame transmission for even lower latency.
- **Per-Stream Filtering and Policing (PSFP, IEEE 802.1Qci)**: Protect
  against babbling nodes or misconfigured traffic.
- **Redundancy (IEEE 802.1CB)**: Frame replication and elimination for
  safety-critical streams, complementing CEELL's existing message redundancy.
