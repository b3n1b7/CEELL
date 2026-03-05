# Pass our custom flash partition layout to the MCUboot image build.
# Using EXTRA_DTC_OVERLAY_FILE preserves MCUboot's own prj.conf defaults.
list(APPEND mcuboot_EXTRA_DTC_OVERLAY_FILE
     ${APP_DIR}/sysbuild/mcuboot_mr_canhubk3.overlay
)
set(mcuboot_EXTRA_DTC_OVERLAY_FILE ${mcuboot_EXTRA_DTC_OVERLAY_FILE}
    CACHE INTERNAL "" FORCE
)

# Merge OTA/MCUboot app-side Kconfig into the main app image
list(APPEND ${ZCMAKE_APPLICATION}_EXTRA_CONF_FILE
     ${APP_DIR}/sysbuild/mcuboot.overlay.conf
)
set(${ZCMAKE_APPLICATION}_EXTRA_CONF_FILE ${${ZCMAKE_APPLICATION}_EXTRA_CONF_FILE}
    CACHE INTERNAL "" FORCE
)
