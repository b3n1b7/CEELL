## Traceability: REQ-SAF-002 REQ-SAF-003 REQ-SAF-004

*** Settings ***
Suite Setup        Setup
Suite Teardown     Teardown
Test Timeout       300 seconds
Resource           s32k_clock_fixes.resource

*** Test Cases ***
Peer Loss Triggers Safe State And Recovery Clears It
    [Documentation]    Verify health transitions and safe state on node loss/recovery
    Execute Command    path add @${CURDIR}/../..
    Execute Command    emulation CreateSwitch "switch"

    # Node 1: Body
    Execute Command    mach create "body"
    Execute Command    machine LoadPlatformDescription @simulation/renode/mr_canhubk3_ceell.repl
    Execute Command    sysbus.emac0 MAC "02:00:00:00:01:10"
    Execute Command    connector Connect sysbus.emac0 switch
    Execute Command    sysbus LoadELF @build/zephyr/zephyr.elf
    Execute Command    sysbus LoadBinary @config/body_config.bin 0x68000000
    Execute Command    cpu0 VectorTableOffset 0x400400
    Apply S32K Clock Fixes
    ${body_uart}=      Create Terminal Tester    sysbus.lpuart2    machine=body
    Execute Command    mach clear

    # Node 2: Chassis
    Execute Command    mach create "chassis"
    Execute Command    machine LoadPlatformDescription @simulation/renode/mr_canhubk3_ceell.repl
    Execute Command    sysbus.emac0 MAC "02:00:00:00:01:11"
    Execute Command    connector Connect sysbus.emac0 switch
    Execute Command    sysbus LoadELF @build/zephyr/zephyr.elf
    Execute Command    sysbus LoadBinary @config/chassis_config.bin 0x68000000
    Execute Command    cpu0 VectorTableOffset 0x400400
    Apply S32K Clock Fixes
    ${chassis_uart}=   Create Terminal Tester    sysbus.lpuart2    machine=chassis
    Execute Command    mach clear

    Start Emulation

    # Wait for mutual discovery
    Wait For Line On Uart    CEELL_HEALTH node_id=1 peers=1    testerId=${body_uart}
    Wait For Line On Uart    CEELL_HEALTH node_id=2 peers=1    testerId=${chassis_uart}

    # Pause chassis — body should detect loss
    Execute Command    mach set "chassis"
    Execute Command    machine Pause

    # Wait for body to detect health degradation and trigger safe state
    Wait For Line On Uart    Peer 2 health:    testerId=${body_uart}
    Wait For Line On Uart    SAFE STATE TRIGGERED    testerId=${body_uart}

    # Resume chassis — body should recover
    Execute Command    mach set "chassis"
    Execute Command    machine Resume

    # Wait for body to clear safe state after chassis recovers
    Wait For Line On Uart    SAFE STATE CLEARED    testerId=${body_uart}
