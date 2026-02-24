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
All Nodes Boot With Flash Config
    [Documentation]    Verify all 3 nodes start with config from flash
    Start Emulation
    Wait For Line On Uart    Config loaded    testerId=body_uart
    Wait For Line On Uart    Config loaded    testerId=chassis_uart
    Wait For Line On Uart    Config loaded    testerId=lights_uart

Body Discovers Chassis And Lights With Services
    [Documentation]    Body node sees both peers with their services
    Wait For Line On Uart    CEELL_DISCOVERED node_id=2    testerId=body_uart
    Wait For Line On Uart    CEELL_DISCOVERED node_id=3    testerId=body_uart

Chassis Discovers Body And Lights With Services
    [Documentation]    Chassis node sees both peers with their services
    Wait For Line On Uart    CEELL_DISCOVERED node_id=1    testerId=chassis_uart
    Wait For Line On Uart    CEELL_DISCOVERED node_id=3    testerId=chassis_uart

Lights Discovers Body And Chassis With Services
    [Documentation]    Lights node sees both peers with their services
    Wait For Line On Uart    CEELL_DISCOVERED node_id=1    testerId=lights_uart
    Wait For Line On Uart    CEELL_DISCOVERED node_id=2    testerId=lights_uart
