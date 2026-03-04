/* REQ-OTA-002 */

#include <zephyr/ztest.h>
#include "ota_version.h"

ZTEST_SUITE(ota_version, NULL, NULL, NULL, NULL, NULL);

/* ── Parsing ─────────────────────────────────────────────────── */

ZTEST(ota_version, test_parse_basic)
{
	struct ceell_ota_ver ver;
	int ret = ceell_ota_ver_parse("1.2.3", &ver);

	zassert_equal(ret, 0);
	zassert_equal(ver.major, 1);
	zassert_equal(ver.minor, 2);
	zassert_equal(ver.patch, 3);
}

ZTEST(ota_version, test_parse_zeros)
{
	struct ceell_ota_ver ver;
	int ret = ceell_ota_ver_parse("0.0.0", &ver);

	zassert_equal(ret, 0);
	zassert_equal(ver.major, 0);
	zassert_equal(ver.minor, 0);
	zassert_equal(ver.patch, 0);
}

ZTEST(ota_version, test_parse_large_patch)
{
	struct ceell_ota_ver ver;
	int ret = ceell_ota_ver_parse("1.0.999", &ver);

	zassert_equal(ret, 0);
	zassert_equal(ver.patch, 999);
}

ZTEST(ota_version, test_parse_null_string)
{
	struct ceell_ota_ver ver;

	zassert_equal(ceell_ota_ver_parse(NULL, &ver), -EINVAL);
}

ZTEST(ota_version, test_parse_null_ver)
{
	zassert_equal(ceell_ota_ver_parse("1.0.0", NULL), -EINVAL);
}

ZTEST(ota_version, test_parse_empty)
{
	struct ceell_ota_ver ver;

	zassert_equal(ceell_ota_ver_parse("", &ver), -EINVAL);
}

ZTEST(ota_version, test_parse_missing_patch)
{
	struct ceell_ota_ver ver;

	zassert_equal(ceell_ota_ver_parse("1.2", &ver), -EINVAL);
}

ZTEST(ota_version, test_parse_letters)
{
	struct ceell_ota_ver ver;

	zassert_equal(ceell_ota_ver_parse("abc", &ver), -EINVAL);
}

/* ── Comparison ──────────────────────────────────────────────── */

ZTEST(ota_version, test_cmp_equal)
{
	struct ceell_ota_ver a = { .major = 1, .minor = 2, .patch = 3 };
	struct ceell_ota_ver b = { .major = 1, .minor = 2, .patch = 3 };

	zassert_equal(ceell_ota_ver_cmp(&a, &b), 0);
}

ZTEST(ota_version, test_cmp_major_greater)
{
	struct ceell_ota_ver a = { .major = 2, .minor = 0, .patch = 0 };
	struct ceell_ota_ver b = { .major = 1, .minor = 9, .patch = 9 };

	zassert_true(ceell_ota_ver_cmp(&a, &b) > 0);
}

ZTEST(ota_version, test_cmp_minor_greater)
{
	struct ceell_ota_ver a = { .major = 1, .minor = 3, .patch = 0 };
	struct ceell_ota_ver b = { .major = 1, .minor = 2, .patch = 999 };

	zassert_true(ceell_ota_ver_cmp(&a, &b) > 0);
}

ZTEST(ota_version, test_cmp_patch_greater)
{
	struct ceell_ota_ver a = { .major = 1, .minor = 2, .patch = 4 };
	struct ceell_ota_ver b = { .major = 1, .minor = 2, .patch = 3 };

	zassert_true(ceell_ota_ver_cmp(&a, &b) > 0);
}

ZTEST(ota_version, test_cmp_less)
{
	struct ceell_ota_ver a = { .major = 0, .minor = 1, .patch = 0 };
	struct ceell_ota_ver b = { .major = 0, .minor = 2, .patch = 0 };

	zassert_true(ceell_ota_ver_cmp(&a, &b) < 0);
}

/* ── Formatting ──────────────────────────────────────────────── */

ZTEST(ota_version, test_to_str)
{
	struct ceell_ota_ver ver = { .major = 0, .minor = 2, .patch = 0 };
	char buf[16];

	ceell_ota_ver_to_str(&ver, buf, sizeof(buf));
	zassert_str_equal(buf, "0.2.0");
}

ZTEST(ota_version, test_to_str_large)
{
	struct ceell_ota_ver ver = { .major = 255, .minor = 255, .patch = 65535 };
	char buf[20];

	ceell_ota_ver_to_str(&ver, buf, sizeof(buf));
	zassert_str_equal(buf, "255.255.65535");
}
