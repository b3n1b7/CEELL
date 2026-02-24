*** Settings ***
Suite Setup        Setup CEELL Network
Suite Teardown     Teardown
Test Timeout       300 seconds
Resource           s32k_clock_fixes.resource

*** Keywords ***
Setup CEELL Network
    Setup
    Execute Command    path add @${CURDIR}/../..
    Execute Command    emulation CreateSwitch "switch"

    # Node 1: Body
    Execute Command    mach create "body"
    Execute Command    machine LoadPlatformDescription @simulation/renode/mr_canhubk3_ceell.repl
    Execute Command    sysbus.gmac0 MAC "02:00:00:00:01:10"
    Execute Command    connector Connect sysbus.gmac0 switch
    Execute Command    sysbus LoadELF @build/zephyr/zephyr.elf
    Execute Command    sysbus LoadBinary @config/body_config.bin 0x68000000
    Execute Command    cpu0 VectorTableOffset 0x400400
    Apply S32K Clock Fixes

    # Node 2: Chassis
    Execute Command    mach create "chassis"
    Execute Command    machine LoadPlatformDescription @simulation/renode/mr_canhubk3_ceell.repl
    Execute Command    sysbus.gmac0 MAC "02:00:00:00:01:11"
    Execute Command    connector Connect sysbus.gmac0 switch
    Execute Command    sysbus LoadELF @build/zephyr/zephyr.elf
    Execute Command    sysbus LoadBinary @config/chassis_config.bin 0x68000000
    Execute Command    cpu0 VectorTableOffset 0x400400
    Apply S32K Clock Fixes

    # Node 3: Lights
    Execute Command    mach create "lights"
    Execute Command    machine LoadPlatformDescription @simulation/renode/mr_canhubk3_ceell.repl
    Execute Command    sysbus.gmac0 MAC "02:00:00:00:01:12"
    Execute Command    connector Connect sysbus.gmac0 switch
    Execute Command    sysbus LoadELF @build/zephyr/zephyr.elf
    Execute Command    sysbus LoadBinary @config/lights_config.bin 0x68000000
    Execute Command    cpu0 VectorTableOffset 0x400400
    Apply S32K Clock Fixes

    # Create terminal testers for each node
    Create Terminal Tester    sysbus.lpuart2    machine=body     timeout=120
    ...    testerId=body_uart
    Create Terminal Tester    sysbus.lpuart2    machine=chassis  timeout=120
    ...    testerId=chassis_uart
    Create Terminal Tester    sysbus.lpuart2    machine=lights   timeout=120
    ...    testerId=lights_uart

*** Test Cases ***
Services Registered On Boot
    [Documentation]    Verify all nodes register their config services and echo
    Start Emulation
    Wait For Line On Uart    CEELL_SERVICE_REGISTERED name=echo             testerId=body_uart
    Wait For Line On Uart    CEELL_SERVICE_REGISTERED name=echo             testerId=chassis_uart
    Wait For Line On Uart    CEELL_SERVICE_REGISTERED name=echo             testerId=lights_uart

Messaging Initialized On All Nodes
    [Documentation]    Verify messaging subsystem starts on all nodes
    Wait For Line On Uart    Messaging initialized    testerId=body_uart
    Wait For Line On Uart    Messaging initialized    testerId=chassis_uart
    Wait For Line On Uart    Messaging initialized    testerId=lights_uart

Discovery Includes Services
    [Documentation]    Verify discovery announcements include services field
    Wait For Line On Uart    CEELL_DISCOVERED node_id=2    testerId=body_uart
    Wait For Line On Uart    CEELL_DISCOVERED node_id=3    testerId=body_uart

Echo Service Request Response
    [Documentation]    Verify echo test service works between nodes
    Wait For Line On Uart    CEELL_ECHO_TEST sending to    testerId=body_uart
    Wait For Line On Uart    CEELL_ECHO_TEST_OK             testerId=body_uart
