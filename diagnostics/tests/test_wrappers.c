#define _XOPEN_SOURCE 700

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>

#include "../src/diag_stats.h"

const struct device test_trackball = {.identifier = 1};
static const struct device other_device = {.identifier = 2};
struct k_msgq input_msgq = {.max_msgs = 8U, .used = 2U};
struct k_msgq zmk_hog_mouse_msgq = {.max_msgs = 4U, .used = 0U};
pthread_mutex_t test_spinlock_mutex = PTHREAD_MUTEX_INITIALIZER;
_Thread_local int test_spinlock_depth;

static int64_t fake_uptime;
static unsigned int uptime_calls;
static int64_t real_input_delay;
static int64_t real_hog_delay;
static int real_input_result;
static int real_hog_result;
static unsigned int input_real_calls;
static unsigned int hog_real_calls;
static const struct device *last_input_dev;
static uint8_t last_input_type;
static uint16_t last_input_code;
static int32_t last_input_value;
static bool last_input_sync;
static k_timeout_t last_input_timeout;
static struct zmk_hid_mouse_report_body *last_hog_report;
static pthread_mutex_t input_gate_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t input_gate_cond = PTHREAD_COND_INITIALIZER;
static bool block_real_input;
static bool real_input_entered;
static bool release_real_input;
static int wrapped_input_result;
static struct diag_window_snapshot during_real_snapshot;

int64_t k_uptime_get(void)
{
	uptime_calls++;
	return fake_uptime;
}

#include "../src/trackball_diag.c"

struct zmk_hid_mouse_report_body {
	int marker;
};

int __real_input_report(const struct device *dev, uint8_t type, uint16_t code,
			int32_t value, bool sync, k_timeout_t timeout)
{
	input_real_calls++;
	last_input_dev = dev;
	last_input_type = type;
	last_input_code = code;
	last_input_value = value;
	last_input_sync = sync;
	last_input_timeout = timeout;
	assert(test_spinlock_depth == 0);
	if (block_real_input) {
		assert(pthread_mutex_lock(&input_gate_mutex) == 0);
		real_input_entered = true;
		assert(pthread_cond_broadcast(&input_gate_cond) == 0);
		while (!release_real_input) {
			assert(pthread_cond_wait(&input_gate_cond, &input_gate_mutex) == 0);
		}
		assert(pthread_mutex_unlock(&input_gate_mutex) == 0);
	}
	fake_uptime += real_input_delay;
	return real_input_result;
}

int __real_zmk_hog_send_mouse_report(struct zmk_hid_mouse_report_body *report)
{
	hog_real_calls++;
	last_hog_report = report;
	fake_uptime += real_hog_delay;
	return real_hog_result;
}

static void test_non_trackball_bypasses_statistics(void)
{
	unsigned int calls_before = input_real_calls;
	unsigned int uptime_before = uptime_calls;
	uint64_t begins_before = stats.operations[DIAG_OP_INPUT_REPORT].total_begun;
	uint64_t samples_before = stats.queues[DIAG_QUEUE_INPUT].total_samples;

	fake_uptime = 17;
	assert(__wrap_input_report(&other_device, 1U, 2U, 3, true, 99) == real_input_result);
	assert(input_real_calls == calls_before + 1U);
	assert(uptime_calls == uptime_before);
	assert(stats.operations[DIAG_OP_INPUT_REPORT].total_begun == begins_before);
	assert(stats.queues[DIAG_QUEUE_INPUT].total_samples == samples_before);
}

static void test_trackball_input_forwards_and_records_result(void)
{
	real_input_delay = 8;
	real_input_result = 0;
	fake_uptime = 100;
	{
		unsigned int uptime_before = uptime_calls;
		input_msgq.used = input_msgq.max_msgs;
		assert(__wrap_input_report(&test_trackball, 4U, 0x123U, -7, false, 1234) == 0);
		assert(uptime_calls == uptime_before + 2U);
	}
	assert(last_input_dev == &test_trackball && last_input_type == 4U);
	assert(last_input_code == 0x123U && last_input_value == -7);
	assert(!last_input_sync && last_input_timeout == 1234);
	assert(stats.operations[DIAG_OP_INPUT_REPORT].total_begun == 1U);
	assert(stats.operations[DIAG_OP_INPUT_REPORT].total_completed == 1U);
	assert(stats.operations[DIAG_OP_INPUT_REPORT].window_buckets[2] == 1U);
	assert(stats.queues[DIAG_QUEUE_INPUT].window_full_samples == 2U);
	assert(stats.queues[DIAG_QUEUE_HOG_MOUSE].window_samples == 2U);

	real_input_delay = 150;
	real_input_result = -EIO;
	fake_uptime = 200;
	input_msgq.used = 3U;
	assert(__wrap_input_report(&test_trackball, 5U, 6U, 7, true, 88) == -EIO);
	assert(stats.operations[DIAG_OP_INPUT_REPORT].total_begun == 2U);
	assert(stats.operations[DIAG_OP_INPUT_REPORT].total_completed == 2U);
	assert(stats.operations[DIAG_OP_INPUT_REPORT].total_errors == 1U);
	assert(stats.operations[DIAG_OP_INPUT_REPORT].window_buckets[4] == 1U);
	assert(stats.operations[DIAG_OP_INPUT_REPORT].window_max_ms == 150U);
}

static void *run_blocked_input_wrapper(void *unused)
{
	(void)unused;
	wrapped_input_result = __wrap_input_report(&test_trackball, 1U, 2U, 3, true, 4);
	return NULL;
}

static void test_sampling_before_and_after_blocked_real_call(void)
{
	pthread_t worker;
	struct diag_window_snapshot before_call = {0};
	struct diag_window_snapshot after_call = {0};
	uint64_t input_begun_before = stats.operations[DIAG_OP_INPUT_REPORT].total_begun;

	/* Start with a clean reporting window. */
	take_snapshot(&before_call);
	input_msgq.used = 2U;
	real_input_delay = 25;
	real_input_result = 0;
	block_real_input = true;
	real_input_entered = false;
	release_real_input = false;
	assert(pthread_create(&worker, NULL, run_blocked_input_wrapper, NULL) == 0);

	assert(pthread_mutex_lock(&input_gate_mutex) == 0);
	while (!real_input_entered) {
		assert(pthread_cond_wait(&input_gate_cond, &input_gate_mutex) == 0);
	}
	assert(pthread_mutex_unlock(&input_gate_mutex) == 0);

	/* The fake real call is still blocked. A reporter snapshot must take the
	 * stats lock and see the before-call queue sample without waiting on it. */
	alarm(5U);
	take_snapshot(&during_real_snapshot);
	alarm(0U);
	assert(during_real_snapshot.operations[DIAG_OP_INPUT_REPORT].window_begun == 1U);
	assert(during_real_snapshot.operations[DIAG_OP_INPUT_REPORT].window_completed == 0U);
	assert(during_real_snapshot.queues[DIAG_QUEUE_INPUT].window_samples == 1U);
	assert(during_real_snapshot.queues[DIAG_QUEUE_INPUT].window_peak_used == 2U);

	input_msgq.used = 7U;
	assert(pthread_mutex_lock(&input_gate_mutex) == 0);
	release_real_input = true;
	assert(pthread_cond_broadcast(&input_gate_cond) == 0);
	assert(pthread_mutex_unlock(&input_gate_mutex) == 0);
	assert(pthread_join(worker, NULL) == 0);
	block_real_input = false;
	assert(wrapped_input_result == 0);
	take_snapshot(&after_call);
	assert(after_call.operations[DIAG_OP_INPUT_REPORT].window_completed == 1U);
	assert(after_call.operations[DIAG_OP_INPUT_REPORT].total_begun == input_begun_before + 1U);
	assert(after_call.operations[DIAG_OP_INPUT_REPORT].total_completed == input_begun_before + 1U);
	assert(after_call.queues[DIAG_QUEUE_INPUT].window_samples == 1U);
	assert(after_call.queues[DIAG_QUEUE_INPUT].window_peak_used == 7U);
}

static void test_hog_forwards_and_records_all_reports(void)
{
	struct zmk_hid_mouse_report_body report = {.marker = 77};

	real_hog_delay = 3;
	real_hog_result = 0;
	fake_uptime = 500;
	zmk_hog_mouse_msgq.used = 4U;
	{
		unsigned int uptime_before = uptime_calls;
		assert(__wrap_zmk_hog_send_mouse_report(&report) == 0);
		assert(uptime_calls == uptime_before + 2U);
	}
	assert(last_hog_report == &report);
	assert(stats.operations[DIAG_OP_HOG_MOUSE_REPORT].total_begun == 1U);
	assert(stats.operations[DIAG_OP_HOG_MOUSE_REPORT].total_completed == 1U);
	assert(stats.operations[DIAG_OP_HOG_MOUSE_REPORT].window_buckets[1] == 1U);
	assert(stats.queues[DIAG_QUEUE_HOG_MOUSE].window_full_samples == 2U);
	assert(stats.queues[DIAG_QUEUE_INPUT].window_samples == 2U);

	real_hog_result = -EIO;
	fake_uptime = 600;
	assert(__wrap_zmk_hog_send_mouse_report(&report) == -EIO);
	assert(stats.operations[DIAG_OP_HOG_MOUSE_REPORT].total_errors == 1U);
}

int main(void)
{
	test_non_trackball_bypasses_statistics();
	test_trackball_input_forwards_and_records_result();
	test_sampling_before_and_after_blocked_real_call();
	test_hog_forwards_and_records_all_reports();
	puts("PASS: wrapper argument/result forwarding, target-device bypass, slow/error accounting and queue sampling");
	return 0;
}
