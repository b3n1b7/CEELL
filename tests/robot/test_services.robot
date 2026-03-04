## Traceability: REQ-MSG-001 REQ-MSG-002 REQ-MSG-003 REQ-MSG-005 REQ-NET-002

*** Settings ***
Suite Setup        Setup
Suite Teardown     Teardown
Test Timeout       300 seconds
Resource           s32k_clock_fixes.resource

*** Test Cases ***
Services And Echo Test Between Nodes
    [Documentation]    Verify service registration, messaging init, discovery with services, and echo send.
    ...                Note: Unicast echo response does not work in Renode due to EMAC SARC
    ...                model limitation (source MAC not replaced in outgoing frames breaks ARP).
    ...                This test verifies the full service layer up to sending the echo request.
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

    # All nodes register echo service
    Wait For Line On Uart    CEELL_SERVICE_REGISTERED name=echo    testerId=${body_uart}
    Wait For Line On Uart    CEELL_SERVICE_REGISTERED name=echo    testerId=${chassis_uart}
    Wait For Line On Uart    CEELL_SERVICE_REGISTERED name=echo    testerId=${lights_uart}

    # Messaging initialized on all
    Wait For Line On Uart    Messaging initialized    testerId=${body_uart}
    Wait For Line On Uart    Messaging initialized    testerId=${chassis_uart}
    Wait For Line On Uart    Messaging initialized    testerId=${lights_uart}

    # All nodes discover each other (order-independent: check health report)
    Wait For Line On Uart    CEELL_HEALTH node_id=1 peers=2    testerId=${body_uart}
    Wait For Line On Uart    CEELL_HEALTH node_id=2 peers=2    testerId=${chassis_uart}
    Wait For Line On Uart    CEELL_HEALTH node_id=3 peers=2    testerId=${lights_uart}

    # Body resolves echo service to a peer and sends request
    # (Response path requires unicast which doesn't work in Renode EMAC model)
    Wait For Line On Uart    CEELL_ECHO_TEST sending to    testerId=${body_uart}
    Wait For Line On Uart    CEELL_MSG_SENT    testerId=${body_uart}
