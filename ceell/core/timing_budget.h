/*
 * CEELL Timing Budget Framework
 *
 * Defines time budgets for critical code paths and checks actual
 * execution time against them. Tracks violation counts for each
 * budget.
 *
 * Behind CONFIG_CEELL_TIMING_BUDGETS (default y), depends on
 * CONFIG_CEELL_TIMING.
 */

#ifndef CEELL_TIMING_BUDGET_H
#define CEELL_TIMING_BUDGET_H

#include <stdint.h>

/**
 * A timing budget definition.
 */
struct ceell_timing_budget {
	const char *label;           /**< Label for this budget */
	uint32_t budget_us;          /**< Budget threshold in microseconds */
	uint32_t violation_count;    /**< Total number of violations seen */
};

/**
 * Register a timing budget for a label.
 *
 * Maximum 16 budgets may be defined.
 *
 * @param label     Label for the budget
 * @param budget_us Maximum acceptable duration in microseconds
 * @return 0 on success, -1 if table full or invalid arguments
 */
int ceell_timing_budget_define(const char *label, uint32_t budget_us);

/**
 * Check actual duration against the budget for a label.
 *
 * @param label     Label to check
 * @param actual_us Measured duration in microseconds
 * @return 0 if within budget, -1 if violation (also increments
 *         violation_count)
 */
int ceell_timing_budget_check(const char *label, uint32_t actual_us);

/**
 * Print all budget definitions with their violation counts.
 * Output goes through ceell_printk.
 */
void ceell_timing_budget_report(void);

#endif /* CEELL_TIMING_BUDGET_H */
