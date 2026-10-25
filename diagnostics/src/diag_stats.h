#ifndef CHARYBDIS_DIAG_STATS_H
#define CHARYBDIS_DIAG_STATS_H

#include <stdbool.h>
#include <stdint.h>

enum diag_operation {
	DIAG_OP_INPUT_REPORT = 0,
	DIAG_OP_HOG_MOUSE_REPORT = 1,
	DIAG_OP_COUNT
};

enum diag_queue {
	DIAG_QUEUE_INPUT = 0,
	DIAG_QUEUE_HOG_MOUSE = 1,
	DIAG_QUEUE_COUNT
};

#define DIAG_LATENCY_BUCKETS 5

struct diag_operation_stats {
	uint64_t window_begun;
	uint64_t window_completed;
	uint64_t window_errors;
	uint64_t total_begun;
	uint64_t total_completed;
	uint64_t total_errors;
	uint64_t window_buckets[DIAG_LATENCY_BUCKETS];
	uint64_t total_buckets[DIAG_LATENCY_BUCKETS];
	uint64_t window_max_ms;
	uint64_t total_max_ms;
	uint64_t window_max_done_uptime_ms;
	uint64_t total_max_done_uptime_ms;
	uint64_t last_complete_uptime_ms;
};

struct diag_queue_stats {
	uint64_t window_samples;
	uint64_t window_full_samples;
	uint64_t total_samples;
	uint64_t total_full_samples;
	uint32_t window_peak_used;
	uint32_t total_peak_used;
	uint32_t capacity;
};

struct diag_stats {
	struct diag_operation_stats operations[DIAG_OP_COUNT];
	struct diag_queue_stats queues[DIAG_QUEUE_COUNT];
};

struct diag_window_snapshot {
	struct diag_operation_stats operations[DIAG_OP_COUNT];
	struct diag_queue_stats queues[DIAG_QUEUE_COUNT];
};

void diag_stats_begin(struct diag_stats *stats, enum diag_operation operation);
void diag_stats_complete(struct diag_stats *stats, enum diag_operation operation,
			 uint64_t started_ms, uint64_t completed_ms, int result);
void diag_stats_queue_sample(struct diag_stats *stats, enum diag_queue queue,
			     uint32_t used, uint32_t capacity);
void diag_stats_take_window(struct diag_stats *stats, struct diag_window_snapshot *snapshot);

#endif
