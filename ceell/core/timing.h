/*
 * CEELL Timing Measurement Framework
 *
 * Ring buffer of timing samples with per-label statistics.
 * Uses hardware cycle counter for sub-microsecond precision.
 * Thread-safe: mutex protects the ring buffer.
 *
 * Behind CONFIG_CEELL_TIMING (default y).
 */

#ifndef CEELL_TIMING_H
#define CEELL_TIMING_H

#include <stdint.h>

/**
 * Timing statistics for a given label.
 */
struct ceell_timing_stats {
	uint32_t min_us;   /**< Minimum observed duration (microseconds) */
	uint32_t max_us;   /**< Maximum observed duration (microseconds) */
	uint32_t avg_us;   /**< Average duration (microseconds) */
	uint32_t count;    /**< Number of samples collected */
};

/**
 * Initialize the timing subsystem.
 * Must be called before any timing start/stop calls.
 */
void ceell_timing_init(void);

/**
 * Record the start of a timed section.
 *
 * @param label  String label identifying this measurement point
 * @param handle Pointer to caller-provided uint32_t that receives
 *               the start cycle count (used by ceell_timing_stop)
 */
void ceell_timing_start(const char *label, uint32_t *handle);

/**
 * Record the end of a timed section.
 *
 * Computes the duration from the handle's start cycle count to now,
 * converts to microseconds, and stores the sample in the ring buffer.
 *
 * @param label  String label (must match the corresponding start call)
 * @param handle The start cycle count returned by ceell_timing_start
 */
void ceell_timing_stop(const char *label, uint32_t handle);

/**
 * Compute statistics from the ring buffer for a given label.
 *
 * Scans all samples in the ring buffer matching the label and
 * computes min, max, average, and count.
 *
 * @param label  Label to compute stats for
 * @param stats  Output structure for statistics
 * @return 0 on success, -1 if no samples found for label
 */
int ceell_timing_get_stats(const char *label, struct ceell_timing_stats *stats);

/**
 * Print statistics for all labels currently in the ring buffer.
 * Output goes through ceell_printk.
 */
void ceell_timing_print_stats(void);

#endif /* CEELL_TIMING_H */
