#include "diag_stats.h"

#include <string.h>

static unsigned int latency_bucket(uint64_t elapsed_ms)
{
	if (elapsed_ms < 1U) {
		return 0U;
	}
	if (elapsed_ms < 5U) {
		return 1U;
	}
	if (elapsed_ms < 20U) {
		return 2U;
	}
	if (elapsed_ms < 100U) {
		return 3U;
	}
	return 4U;
}

void diag_stats_begin(struct diag_stats *stats, enum diag_operation operation)
{
	struct diag_operation_stats *op = &stats->operations[operation];

	op->window_begun++;
	op->total_begun++;
}

void diag_stats_complete(struct diag_stats *stats, enum diag_operation operation,
			 uint64_t started_ms, uint64_t completed_ms, int result)
{
	struct diag_operation_stats *op = &stats->operations[operation];
	uint64_t elapsed_ms = completed_ms - started_ms;
	unsigned int bucket = latency_bucket(elapsed_ms);

	op->window_completed++;
	op->total_completed++;
	op->window_buckets[bucket]++;
	op->total_buckets[bucket]++;
	op->last_complete_uptime_ms = completed_ms;
	if (elapsed_ms > op->window_max_ms) {
		op->window_max_ms = elapsed_ms;
		op->window_max_done_uptime_ms = completed_ms;
	}
	if (elapsed_ms > op->total_max_ms) {
		op->total_max_ms = elapsed_ms;
		op->total_max_done_uptime_ms = completed_ms;
	}
	if (result != 0) {
		op->window_errors++;
		op->total_errors++;
	}
}

void diag_stats_queue_sample(struct diag_stats *stats, enum diag_queue queue,
			     uint32_t used, uint32_t capacity)
{
	struct diag_queue_stats *q = &stats->queues[queue];

	q->window_samples++;
	q->total_samples++;
	q->capacity = capacity;
	if (used > q->window_peak_used) {
		q->window_peak_used = used;
	}
	if (used > q->total_peak_used) {
		q->total_peak_used = used;
	}
	if (used >= capacity) {
		q->window_full_samples++;
		q->total_full_samples++;
	}
}

void diag_stats_take_window(struct diag_stats *stats, struct diag_window_snapshot *snapshot)
{
	unsigned int i;

	memset(snapshot, 0, sizeof(*snapshot));
	for (i = 0U; i < DIAG_OP_COUNT; i++) {
		snapshot->operations[i] = stats->operations[i];
		stats->operations[i].window_begun = 0U;
		stats->operations[i].window_completed = 0U;
		stats->operations[i].window_errors = 0U;
		memset(stats->operations[i].window_buckets, 0,
		       sizeof(stats->operations[i].window_buckets));
		stats->operations[i].window_max_ms = 0U;
		stats->operations[i].window_max_done_uptime_ms = 0U;
	}
	for (i = 0U; i < DIAG_QUEUE_COUNT; i++) {
		snapshot->queues[i] = stats->queues[i];
		stats->queues[i].window_samples = 0U;
		stats->queues[i].window_full_samples = 0U;
		stats->queues[i].window_peak_used = 0U;
	}
}
