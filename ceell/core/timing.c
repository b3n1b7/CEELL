/*
 * CEELL Timing Measurement Framework — Implementation
 *
 * Ring buffer of timing samples with per-label statistics.
 * Converts hardware cycles to microseconds using the configured
 * system clock frequency (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC).
 */

#include "timing.h"
#include "osal.h"

#include <string.h>

#ifndef CONFIG_CEELL_TIMING_SAMPLES
#define CONFIG_CEELL_TIMING_SAMPLES 64
#endif

#ifndef CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
#define CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC 1000000
#endif

CEELL_LOG_MODULE_REGISTER(ceell_timing, CEELL_LOG_LEVEL_INF);

/* ── Internal types ─────────────────────────────────────────────── */

struct timing_sample {
	const char *label;
	uint32_t start_cycle;
	uint32_t end_cycle;
	uint32_t duration_us;
};

/* ── Module state ───────────────────────────────────────────────── */

static struct timing_sample samples[CONFIG_CEELL_TIMING_SAMPLES];
static uint32_t sample_head;       /* Next write index (wraps) */
static uint32_t sample_count;      /* Total samples stored (capped at ring size) */
static ceell_mutex_t timing_mutex;
static bool initialized;

/* ── Helpers ────────────────────────────────────────────────────── */

static uint32_t cycles_to_us(uint32_t cycles)
{
	/* cycles / (cycles_per_sec / 1000000) = microseconds */
	uint64_t us = (uint64_t)cycles * 1000000ULL /
		      (uint64_t)CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
	return (uint32_t)us;
}

/* ── Public API ─────────────────────────────────────────────────── */

void ceell_timing_init(void)
{
	ceell_mutex_init(&timing_mutex);
	memset(samples, 0, sizeof(samples));
	sample_head = 0;
	sample_count = 0;
	initialized = true;
	CEELL_LOG_INF("Timing subsystem initialized (%d sample slots)",
		      CONFIG_CEELL_TIMING_SAMPLES);
}

void ceell_timing_start(const char *label, uint32_t *handle)
{
	ARG_UNUSED(label);

	if (handle == NULL) {
		return;
	}
	*handle = ceell_cycle_get();
}

void ceell_timing_stop(const char *label, uint32_t handle)
{
	uint32_t end_cycle;
	uint32_t duration_cycles;
	struct timing_sample *s;

	if (!initialized || label == NULL) {
		return;
	}

	end_cycle = ceell_cycle_get();

	/* Handle wrap-around: unsigned subtraction is correct for
	 * monotonically-increasing counters that wrap at 2^32. */
	duration_cycles = end_cycle - handle;

	ceell_mutex_lock(&timing_mutex);

	s = &samples[sample_head % CONFIG_CEELL_TIMING_SAMPLES];
	s->label = label;
	s->start_cycle = handle;
	s->end_cycle = end_cycle;
	s->duration_us = cycles_to_us(duration_cycles);

	sample_head = (sample_head + 1) % CONFIG_CEELL_TIMING_SAMPLES;
	if (sample_count < CONFIG_CEELL_TIMING_SAMPLES) {
		sample_count++;
	}

	ceell_mutex_unlock(&timing_mutex);
}

int ceell_timing_get_stats(const char *label, struct ceell_timing_stats *stats)
{
	uint32_t min_us = UINT32_MAX;
	uint32_t max_us = 0;
	uint64_t sum_us = 0;
	uint32_t count = 0;
	uint32_t i;

	if (label == NULL || stats == NULL) {
		return -1;
	}

	if (!initialized) {
		return -1;
	}

	ceell_mutex_lock(&timing_mutex);

	for (i = 0; i < sample_count; i++) {
		const struct timing_sample *s = &samples[i];

		if (s->label != NULL && strcmp(s->label, label) == 0) {
			if (s->duration_us < min_us) {
				min_us = s->duration_us;
			}
			if (s->duration_us > max_us) {
				max_us = s->duration_us;
			}
			sum_us += s->duration_us;
			count++;
		}
	}

	ceell_mutex_unlock(&timing_mutex);

	if (count == 0) {
		return -1;
	}

	stats->min_us = min_us;
	stats->max_us = max_us;
	stats->avg_us = (uint32_t)(sum_us / count);
	stats->count = count;

	return 0;
}

void ceell_timing_print_stats(void)
{
	/* Collect unique labels from the ring buffer */
	const char *seen_labels[CONFIG_CEELL_TIMING_SAMPLES];
	uint32_t num_labels = 0;
	uint32_t i;
	uint32_t j;

	if (!initialized) {
		ceell_printk("Timing: not initialized\n");
		return;
	}

	ceell_mutex_lock(&timing_mutex);

	/* Build list of unique labels */
	for (i = 0; i < sample_count; i++) {
		const char *lbl = samples[i].label;
		bool found = false;

		if (lbl == NULL) {
			continue;
		}

		for (j = 0; j < num_labels; j++) {
			if (strcmp(seen_labels[j], lbl) == 0) {
				found = true;
				break;
			}
		}
		if (!found && num_labels < CONFIG_CEELL_TIMING_SAMPLES) {
			seen_labels[num_labels++] = lbl;
		}
	}

	ceell_mutex_unlock(&timing_mutex);

	/* Print stats for each unique label */
	ceell_printk("=== Timing Statistics ===\n");
	for (i = 0; i < num_labels; i++) {
		struct ceell_timing_stats stats;

		if (ceell_timing_get_stats(seen_labels[i], &stats) == 0) {
			ceell_printk("  %-20s  min=%u us  max=%u us  avg=%u us  n=%u\n",
				     seen_labels[i],
				     stats.min_us, stats.max_us,
				     stats.avg_us, stats.count);
		}
	}
	ceell_printk("=========================\n");
}
