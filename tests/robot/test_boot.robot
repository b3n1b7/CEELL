## Traceability: REQ-SAF-001 REQ-SAF-002 REQ-SAF-004 REQ-MSG-004 REQ-NET-001 REQ-OSAL-005

*** Settings ***
Suite Setup        Setup
Suite Teardown     Teardown
Test Timeout       120 seconds
Resource           s32k_clock_fixes.resource

*** Test Cases ***
CEELL Boots And Loads Flash Config
    [Documentation]    Verify single node boots, loads config from flash, registers services, and starts messaging
    Execute Command    path add @${CURDIR}/../..
    Execute Command    mach create "body"
    Execute Command    machine LoadPlatformDescription @simulation/renode/mr_canhubk3_ceell.repl
    Execute Command    sysbus.emac0 MAC "02:00:00:00:01:10"
    Execute Command    sysbus LoadELF @build/zephyr/zephyr.elf
    Execute Command    sysbus LoadBinary @config/body_config.bin 0x68000000
    Execute Command    cpu0 VectorTableOffset 0x400400
    Apply S32K Clock Fixes
    Create Terminal Tester    sysbus.lpuart2    timeout=60

    Start Emulation
    Wait For Line On Uart    CEELL middleware starting
    Wait For Line On Uart    Config loaded
    Wait For Line On Uart    Identity set
    Wait For Line On Uart    Service registry initialized
    Wait For Line On Uart    CEELL_SERVICE_REGISTERED
    Wait For Line On Uart    Timing subsystem initialized
    Wait For Line On Uart    Health monitor initialized
    Wait For Line On Uart    Discovery initialized
    Wait For Line On Uart    Discovery threads started
    Wait For Line On Uart    Messaging initialized
