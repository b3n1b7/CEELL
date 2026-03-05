# Pass our custom flash partition layout to the MCUboot image build.
# Using EXTRA_DTC_OVERLAY_FILE preserves MCUboot's own prj.conf defaults.
list(APPEND mcuboot_EXTRA_DTC_OVERLAY_FILE
     ${APP_DIR}/sysbuild/mcuboot_mr_canhubk3.overlay
)
set(mcuboot_EXTRA_DTC_OVERLAY_FILE ${mcuboot_EXTRA_DTC_OVERLAY_FILE}
    CACHE INTERNAL "" FORCE
)

# Pass MCUboot flash partitions to the app image.
# This overlay adds boot/slot0/slot1/scratch on top of the base board overlay.
# File is NOT named ceell-app.overlay to avoid sysbuild auto-detection
# which would replace the auto-detected boards/mr_canhubk3.overlay.
list(APPEND ceell-app_EXTRA_DTC_OVERLAY_FILE
     ${APP_DIR}/sysbuild/ceell-app_mr_canhubk3.overlay
)
set(ceell-app_EXTRA_DTC_OVERLAY_FILE ${ceell-app_EXTRA_DTC_OVERLAY_FILE}
    CACHE INTERNAL "" FORCE
)

# App-side OTA/MCUboot Kconfig is auto-detected from sysbuild/ceell-app.conf
