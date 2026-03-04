/*
 * CEELL Timing Budget Framework — Implementation
 *
 * Manages up to 16 named timing budgets.  When a measured duration
 * exceeds its budget the violation counter is incremented.  Repeated
 * critical-path violations (3+ consecutive) would trigger safe state
 * in a future Phase 7 integration; for now we only log a warning.
 */

#include "timing_budget.h"
#include "osal.h"

#include <string.h>

#define CEELL_TIMING_BUDGET_MAX 16

/* Consecutive violation threshold that would trigger safe state */
#define CONSECUTIVE_VIOLATION_THRESHOLD 3

CEELL_LOG_MODULE_REGISTER(ceell_timing_budget, CEELL_LOG_LEVEL_INF);

/* ── Module state ───────────────────────────────────────────────── */

static struct ceell_timing_budget budgets[CEELL_TIMING_BUDGET_MAX];
static uint32_t budget_count;

/* Track consecutive violations per budget for safe-state trigger */
static uint32_t consecutive_violations[CEELL_TIMING_BUDGET_MAX];

static ceell_mutex_t budget_mutex;
static bool initialized;

/* ── Helpers ────────────────────────────────────────────────────── */

static void ensure_init(void)
{
	if (!initialized) {
		ceell_mutex_init(&budget_mutex);
		memset(budgets, 0, sizeof(budgets));
		memset(consecutive_violations, 0, sizeof(consecutive_violations));
		budget_count = 0;
		initialized = true;
	}
}

static int find_budget(const char *label)
{
	uint32_t i;

	for (i = 0; i < budget_count; i++) {
		if (budgets[i].label != NULL &&
		    strcmp(budgets[i].label, label) == 0) {
			return (int)i;
		}
	}
	return -1;
}

/* ── Public API ─────────────────────────────────────────────────── */

int ceell_timing_budget_define(const char *label, uint32_t budget_us)
{
	int idx;

	if (label == NULL || budget_us == 0) {
		return -1;
	}

	ensure_init();

	ceell_mutex_lock(&budget_mutex);

	/* Check if label already exists */
	idx = find_budget(label);
	if (idx >= 0) {
		/* Update existing budget */
		budgets[idx].budget_us = budget_us;
		ceell_mutex_unlock(&budget_mutex);
		return 0;
	}

	if (budget_count >= CEELL_TIMING_BUDGET_MAX) {
		ceell_mutex_unlock(&budget_mutex);
		CEELL_LOG_ERR("Budget table full (%d/%d)",
			      budget_count, CEELL_TIMING_BUDGET_MAX);
		return -1;
	}

	budgets[budget_count].label = label;
	budgets[budget_count].budget_us = budget_us;
	budgets[budget_count].violation_count = 0;
	consecutive_violations[budget_count] = 0;
	budget_count++;

	ceell_mutex_unlock(&budget_mutex);

	CEELL_LOG_INF("Budget defined: %s = %u us", label, budget_us);
	return 0;
}

int ceell_timing_budget_check(const char *label, uint32_t actual_us)
{
	int idx;

	if (label == NULL) {
		return -1;
	}

	ensure_init();

	ceell_mutex_lock(&budget_mutex);

	idx = find_budget(label);
	if (idx < 0) {
		ceell_mutex_unlock(&budget_mutex);
		return -1;
	}

	if (actual_us > budgets[idx].budget_us) {
		budgets[idx].violation_count++;
		consecutive_violations[idx]++;

		if (consecutive_violations[idx] >= CONSECUTIVE_VIOLATION_THRESHOLD) {
			CEELL_LOG_WRN("Budget '%s': %u consecutive violations "
				      "(would trigger safe state)",
				      label, consecutive_violations[idx]);
		}

		ceell_mutex_unlock(&budget_mutex);
		return -1;
	}

	/* Within budget — reset consecutive violation counter */
	consecutive_violations[idx] = 0;
	ceell_mutex_unlock(&budget_mutex);
	return 0;
}

void ceell_timing_budget_report(void)
{
	uint32_t i;

	ensure_init();

	ceell_mutex_lock(&budget_mutex);

	ceell_printk("=== Timing Budget Report ===\n");
	for (i = 0; i < budget_count; i++) {
		ceell_printk("  %-20s  budget=%u us  violations=%u\n",
			     budgets[i].label,
			     budgets[i].budget_us,
			     budgets[i].violation_count);
	}
	ceell_printk("============================\n");

	ceell_mutex_unlock(&budget_mutex);
}
