*** Keywords ***
Apply S32K Clock Fixes
    [Documentation]    Fix clock/QSPI polling loops for fast Renode boot on S32K3xx
    # Pre-populate clock status registers
    Execute Command    sysbus WriteDoubleWord 0x402D0004 0x00000001
    Execute Command    sysbus WriteDoubleWord 0x402D4004 0x80000000
    Execute Command    sysbus WriteDoubleWord 0x402CC004 0x80000000
    Execute Command    sysbus WriteDoubleWord 0x402E0004 0x00000004
    # Replace CGM MUX CSS flipflop peripherals with constant value (SWTRG=001)
    FOR    ${i}    IN RANGE    20
        Execute Command    sysbus Unregister sysbus.mc_cgm_mux_${i}_css
    END
    Execute Command    machine PyDevFromString "request.Value = 0x00020004" 0x402D8304 0x4 true "mc_cgm_mux_0_css"
    Execute Command    machine PyDevFromString "request.Value = 0x00020004" 0x402D8344 0x4 true "mc_cgm_mux_1_css"
    Execute Command    machine PyDevFromString "request.Value = 0x00020004" 0x402D8384 0x4 true "mc_cgm_mux_2_css"
    Execute Command    machine PyDevFromString "request.Value = 0x00020004" 0x402D83C4 0x4 true "mc_cgm_mux_3_css"
    Execute Command    machine PyDevFromString "request.Value = 0x00020004" 0x402D8404 0x4 true "mc_cgm_mux_4_css"
    Execute Command    machine PyDevFromString "request.Value = 0x00020004" 0x402D8444 0x4 true "mc_cgm_mux_5_css"
    Execute Command    machine PyDevFromString "request.Value = 0x00020004" 0x402D8484 0x4 true "mc_cgm_mux_6_css"
    Execute Command    machine PyDevFromString "request.Value = 0x00020004" 0x402D84C4 0x4 true "mc_cgm_mux_7_css"
    Execute Command    machine PyDevFromString "request.Value = 0x00020004" 0x402D8504 0x4 true "mc_cgm_mux_8_css"
    Execute Command    machine PyDevFromString "request.Value = 0x00020004" 0x402D8544 0x4 true "mc_cgm_mux_9_css"
    Execute Command    machine PyDevFromString "request.Value = 0x00020004" 0x402D8584 0x4 true "mc_cgm_mux_10_css"
    Execute Command    machine PyDevFromString "request.Value = 0x00020004" 0x402D85C4 0x4 true "mc_cgm_mux_11_css"
    Execute Command    machine PyDevFromString "request.Value = 0x00020004" 0x402D8604 0x4 true "mc_cgm_mux_12_css"
    Execute Command    machine PyDevFromString "request.Value = 0x00020004" 0x402D8644 0x4 true "mc_cgm_mux_13_css"
    Execute Command    machine PyDevFromString "request.Value = 0x00020004" 0x402D8684 0x4 true "mc_cgm_mux_14_css"
    Execute Command    machine PyDevFromString "request.Value = 0x00020004" 0x402D86C4 0x4 true "mc_cgm_mux_15_css"
    Execute Command    machine PyDevFromString "request.Value = 0x00020004" 0x402D8704 0x4 true "mc_cgm_mux_16_css"
    Execute Command    machine PyDevFromString "request.Value = 0x00020004" 0x402D8744 0x4 true "mc_cgm_mux_17_css"
    Execute Command    machine PyDevFromString "request.Value = 0x00020004" 0x402D8784 0x4 true "mc_cgm_mux_18_css"
    Execute Command    machine PyDevFromString "request.Value = 0x00020004" 0x402D87C4 0x4 true "mc_cgm_mux_19_css"
    # Fix QuadSPI DLL-A Slave Lock status (bits 14+15 = locked)
    Execute Command    machine PyDevFromString "request.Value = 0x0000C000" 0x404CC12C 0x4 true "qspi_dllsr"
