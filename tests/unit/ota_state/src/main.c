/* REQ-OTA-001 */
/* REQ-SAF-005 */

#include <zephyr/ztest.h>
#include "ota_state.h"

ZTEST_SUITE(ota_state, NULL, NULL, NULL, NULL, NULL);

/* ── Initial state ───────────────────────────────────────────── */

ZTEST(ota_state, test_initial_state_is_idle)
{
	zassert_equal(ceell_ota_get_state(), CEELL_OTA_IDLE);
}

/* ── State transitions ───────────────────────────────────────── */

ZTEST(ota_state, test_set_checking)
{
	ceell_ota_set_state(CEELL_OTA_CHECKING);
	zassert_equal(ceell_ota_get_state(), CEELL_OTA_CHECKING);

	/* Reset for other tests */
	ceell_ota_set_state(CEELL_OTA_IDLE);
}

ZTEST(ota_state, test_set_downloading)
{
	ceell_ota_set_state(CEELL_OTA_DOWNLOADING);
	zassert_equal(ceell_ota_get_state(), CEELL_OTA_DOWNLOADING);

	ceell_ota_set_state(CEELL_OTA_IDLE);
}

ZTEST(ota_state, test_set_error)
{
	ceell_ota_set_state(CEELL_OTA_ERROR);
	zassert_equal(ceell_ota_get_state(), CEELL_OTA_ERROR);

	ceell_ota_set_state(CEELL_OTA_IDLE);
}

ZTEST(ota_state, test_full_happy_path_transitions)
{
	ceell_ota_set_state(CEELL_OTA_CHECKING);
	zassert_equal(ceell_ota_get_state(), CEELL_OTA_CHECKING);

	ceell_ota_set_state(CEELL_OTA_DOWNLOADING);
	zassert_equal(ceell_ota_get_state(), CEELL_OTA_DOWNLOADING);

	ceell_ota_set_state(CEELL_OTA_VALIDATING);
	zassert_equal(ceell_ota_get_state(), CEELL_OTA_VALIDATING);

	ceell_ota_set_state(CEELL_OTA_MARKING);
	zassert_equal(ceell_ota_get_state(), CEELL_OTA_MARKING);

	ceell_ota_set_state(CEELL_OTA_REBOOTING);
	zassert_equal(ceell_ota_get_state(), CEELL_OTA_REBOOTING);

	ceell_ota_set_state(CEELL_OTA_IDLE);
}

/* ── String representations ──────────────────────────────────── */

ZTEST(ota_state, test_str_idle)
{
	zassert_str_equal(ceell_ota_state_str(CEELL_OTA_IDLE), "idle");
}

ZTEST(ota_state, test_str_checking)
{
	zassert_str_equal(ceell_ota_state_str(CEELL_OTA_CHECKING), "checking");
}

ZTEST(ota_state, test_str_downloading)
{
	zassert_str_equal(ceell_ota_state_str(CEELL_OTA_DOWNLOADING), "downloading");
}

ZTEST(ota_state, test_str_validating)
{
	zassert_str_equal(ceell_ota_state_str(CEELL_OTA_VALIDATING), "validating");
}

ZTEST(ota_state, test_str_marking)
{
	zassert_str_equal(ceell_ota_state_str(CEELL_OTA_MARKING), "marking");
}

ZTEST(ota_state, test_str_rebooting)
{
	zassert_str_equal(ceell_ota_state_str(CEELL_OTA_REBOOTING), "rebooting");
}

ZTEST(ota_state, test_str_error)
{
	zassert_str_equal(ceell_ota_state_str(CEELL_OTA_ERROR), "error");
}

ZTEST(ota_state, test_str_unknown)
{
	zassert_str_equal(ceell_ota_state_str((enum ceell_ota_state)99), "unknown");
}
