## Traceability: REQ-TIM-001 REQ-TIM-002

*** Settings ***
Suite Setup        Setup
Suite Teardown     Teardown
Test Timeout       300 seconds
Resource           s32k_clock_fixes.resource

*** Test Cases ***
Timing Statistics Printed After Lifecycle Ticks
    [Documentation]    Verify timing stats appear on UART after 6 lifecycle ticks (~30s)
    Execute Command    path add @${CURDIR}/../..
    Execute Command    mach create "body"
    Execute Command    machine LoadPlatformDescription @simulation/renode/mr_canhubk3_ceell.repl
    Execute Command    sysbus.emac0 MAC "02:00:00:00:01:10"
    Execute Command    sysbus LoadELF @build/zephyr/zephyr.elf
    Execute Command    sysbus LoadBinary @config/body_config.bin 0x68000000
    Execute Command    cpu0 VectorTableOffset 0x400400
    Apply S32K Clock Fixes
    Create Terminal Tester    sysbus.lpuart2    timeout=120

    Start Emulation
    Wait For Line On Uart    Messaging initialized
    Wait For Line On Uart    === Timing Statistics ===
    Wait For Line On Uart    lifecycle_tick
    Wait For Line On Uart    =========================
