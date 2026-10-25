#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "diag_stats.h"

static void test_latency_buckets_and_errors(void)
{
	struct diag_stats stats = {0};

	diag_stats_begin(&stats, DIAG_OP_INPUT_REPORT);
	diag_stats_complete(&stats, DIAG_OP_INPUT_REPORT, 100U, 100U, 0);
	diag_stats_begin(&stats, DIAG_OP_INPUT_REPORT);
	diag_stats_complete(&stats, DIAG_OP_INPUT_REPORT, 100U, 101U, -5);
	diag_stats_begin(&stats, DIAG_OP_INPUT_REPORT);
	diag_stats_complete(&stats, DIAG_OP_INPUT_REPORT, 100U, 105U, 0);
	diag_stats_begin(&stats, DIAG_OP_INPUT_REPORT);
	diag_stats_complete(&stats, DIAG_OP_INPUT_REPORT, 105U, 125U, 0);
	diag_stats_begin(&stats, DIAG_OP_INPUT_REPORT);
	diag_stats_complete(&stats, DIAG_OP_INPUT_REPORT, 125U, 225U, 0);
	diag_stats_begin(&stats, DIAG_OP_INPUT_REPORT);
	diag_stats_complete(&stats, DIAG_OP_INPUT_REPORT, 225U, 226U, 0);

	assert(stats.operations[DIAG_OP_INPUT_REPORT].window_begun == 6U);
	assert(stats.operations[DIAG_OP_INPUT_REPORT].window_completed == 6U);
	assert(stats.operations[DIAG_OP_INPUT_REPORT].window_errors == 1U);
	assert(stats.operations[DIAG_OP_INPUT_REPORT].window_buckets[0] == 1U);
	assert(stats.operations[DIAG_OP_INPUT_REPORT].window_buckets[1] == 2U);
	assert(stats.operations[DIAG_OP_INPUT_REPORT].window_buckets[2] == 1U);
	assert(stats.operations[DIAG_OP_INPUT_REPORT].window_buckets[3] == 1U);
	assert(stats.operations[DIAG_OP_INPUT_REPORT].window_buckets[4] == 1U);
	assert(stats.operations[DIAG_OP_INPUT_REPORT].window_max_ms == 100U);
	assert(stats.operations[DIAG_OP_INPUT_REPORT].window_max_done_uptime_ms == 225U);
	assert(stats.operations[DIAG_OP_INPUT_REPORT].last_complete_uptime_ms == 226U);
}

static void test_cross_window_accounting(void)
{
	struct diag_stats stats = {0};
	struct diag_window_snapshot first = {0};
	struct diag_window_snapshot second = {0};

	diag_stats_begin(&stats, DIAG_OP_HOG_MOUSE_REPORT);
	diag_stats_take_window(&stats, &first);
	assert(first.operations[DIAG_OP_HOG_MOUSE_REPORT].window_begun == 1U);
	assert(first.operations[DIAG_OP_HOG_MOUSE_REPORT].window_completed == 0U);

	diag_stats_complete(&stats, DIAG_OP_HOG_MOUSE_REPORT, 7U, 11U, 0);
	diag_stats_take_window(&stats, &second);
	assert(second.operations[DIAG_OP_HOG_MOUSE_REPORT].window_begun == 0U);
	assert(second.operations[DIAG_OP_HOG_MOUSE_REPORT].window_completed == 1U);
	assert(second.operations[DIAG_OP_HOG_MOUSE_REPORT].total_begun == 1U);
	assert(second.operations[DIAG_OP_HOG_MOUSE_REPORT].total_completed == 1U);
}

static void test_queue_samples_and_wraparound_time(void)
{
	struct diag_stats stats = {0};
	struct diag_window_snapshot snapshot = {0};

	diag_stats_queue_sample(&stats, DIAG_QUEUE_INPUT, 2U, 8U);
	diag_stats_queue_sample(&stats, DIAG_QUEUE_INPUT, 8U, 8U);
	diag_stats_queue_sample(&stats, DIAG_QUEUE_HOG_MOUSE, 1U, 4U);
	diag_stats_begin(&stats, DIAG_OP_INPUT_REPORT);
	diag_stats_complete(&stats, DIAG_OP_INPUT_REPORT, UINT64_MAX - 2U, 3U, 0);
	diag_stats_take_window(&stats, &snapshot);

	assert(snapshot.queues[DIAG_QUEUE_INPUT].window_samples == 2U);
	assert(snapshot.queues[DIAG_QUEUE_INPUT].window_full_samples == 1U);
	assert(snapshot.queues[DIAG_QUEUE_INPUT].window_peak_used == 8U);
	assert(snapshot.queues[DIAG_QUEUE_INPUT].capacity == 8U);
	assert(snapshot.queues[DIAG_QUEUE_HOG_MOUSE].window_samples == 1U);
	assert(snapshot.operations[DIAG_OP_INPUT_REPORT].window_buckets[2] == 1U);
	assert(snapshot.operations[DIAG_OP_INPUT_REPORT].window_max_ms == 6U);
}

int main(void)
{
	test_latency_buckets_and_errors();
	test_cross_window_accounting();
	test_queue_samples_and_wraparound_time();
	puts("PASS: diagnostic statistics buckets, errors, cross-window totals, queue samples and unsigned time wrap");
	return 0;
}
