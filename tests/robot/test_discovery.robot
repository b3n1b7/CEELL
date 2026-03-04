## Traceability: REQ-NET-001 REQ-NET-003 REQ-SAF-002 REQ-SAF-003

*** Settings ***
Suite Setup        Setup
Suite Teardown     Teardown
Test Timeout       300 seconds
Resource           s32k_clock_fixes.resource

*** Test Cases ***
Three Nodes Discover Each Other
    [Documentation]    Verify 3 nodes boot, discover each other with services
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

    # Node 3: Lights
    Execute Command    mach create "lights"
    Execute Command    machine LoadPlatformDescription @simulation/renode/mr_canhubk3_ceell.repl
    Execute Command    sysbus.emac0 MAC "02:00:00:00:01:12"
    Execute Command    connector Connect sysbus.emac0 switch
    Execute Command    sysbus LoadELF @build/zephyr/zephyr.elf
    Execute Command    sysbus LoadBinary @config/lights_config.bin 0x68000000
    Execute Command    cpu0 VectorTableOffset 0x400400
    Apply S32K Clock Fixes
    ${lights_uart}=    Create Terminal Tester    sysbus.lpuart2    machine=lights
    Execute Command    mach clear

    Start Emulation

    # All nodes boot
    Wait For Line On Uart    Config loaded    testerId=${body_uart}
    Wait For Line On Uart    Config loaded    testerId=${chassis_uart}
    Wait For Line On Uart    Config loaded    testerId=${lights_uart}

    # Each node discovers both peers (order-independent: check health report)
    Wait For Line On Uart    CEELL_HEALTH node_id=1 peers=2    testerId=${body_uart}
    Wait For Line On Uart    CEELL_HEALTH node_id=2 peers=2    testerId=${chassis_uart}
    Wait For Line On Uart    CEELL_HEALTH node_id=3 peers=2    testerId=${lights_uart}
