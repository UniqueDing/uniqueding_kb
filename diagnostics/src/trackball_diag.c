#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "diag_stats.h"

#if !defined(CONFIG_CHARYBDIS_TRACKBALL_DIAGNOSTICS) || \
	!CONFIG_CHARYBDIS_TRACKBALL_DIAGNOSTICS
#error "trackball_diag.c must only be compiled with diagnostics enabled"
#endif
#if !defined(CONFIG_INPUT_MODE_THREAD) || !CONFIG_INPUT_MODE_THREAD
#error "trackball diagnostics require CONFIG_INPUT_MODE_THREAD=y"
#endif
#if !defined(CONFIG_ZMK_BLE) || !CONFIG_ZMK_BLE
#error "trackball diagnostics require CONFIG_ZMK_BLE=y"
#endif
#if !defined(CONFIG_ZMK_STUDIO) || !CONFIG_ZMK_STUDIO
#error "trackball diagnostics require CONFIG_ZMK_STUDIO=y"
#endif
#if !defined(CONFIG_ZMK_STUDIO_TRANSPORT_BLE) || !CONFIG_ZMK_STUDIO_TRANSPORT_BLE
#error "trackball diagnostics require CONFIG_ZMK_STUDIO_TRANSPORT_BLE=y"
#endif
#if defined(CONFIG_ZMK_STUDIO_TRANSPORT_UART) && CONFIG_ZMK_STUDIO_TRANSPORT_UART
#error "trackball diagnostics require CONFIG_ZMK_STUDIO_TRANSPORT_UART=n"
#endif
#if defined(CONFIG_ZMK_USB) && CONFIG_ZMK_USB
#error "trackball diagnostics require CONFIG_ZMK_USB=n"
#endif
#if !defined(CONFIG_ZMK_POINTING) || !CONFIG_ZMK_POINTING
#error "trackball diagnostics require CONFIG_ZMK_POINTING=y"
#endif
#if !defined(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) || !CONFIG_ZMK_SPLIT_ROLE_CENTRAL
#error "trackball diagnostics require the ZMK split central role"
#endif
#if !defined(CONFIG_LOG) || !CONFIG_LOG || !defined(CONFIG_LOG_MODE_DEFERRED) || \
	!CONFIG_LOG_MODE_DEFERRED
#error "trackball diagnostics require deferred Zephyr logging"
#endif
#if !defined(CONFIG_ZMK_LOGGING_MINIMAL) || !CONFIG_ZMK_LOGGING_MINIMAL
#error "trackball diagnostics require CONFIG_ZMK_LOGGING_MINIMAL=y"
#endif
#if !defined(CONFIG_ZMK_LOG_LEVEL_OFF) || !CONFIG_ZMK_LOG_LEVEL_OFF
#error "trackball diagnostics require CONFIG_ZMK_LOG_LEVEL_OFF=y"
#endif
#if !defined(CONFIG_LOG_DEFAULT_LEVEL) || CONFIG_LOG_DEFAULT_LEVEL != 0
#error "trackball diagnostics require CONFIG_LOG_DEFAULT_LEVEL=0"
#endif
#if defined(CONFIG_INPUT_EVENT_DUMP) && CONFIG_INPUT_EVENT_DUMP
#error "trackball diagnostics require CONFIG_INPUT_EVENT_DUMP=n"
#endif
#if !defined(CONFIG_INPUT_LOG_LEVEL_OFF) || !CONFIG_INPUT_LOG_LEVEL_OFF
#error "trackball diagnostics require CONFIG_INPUT_LOG_LEVEL_OFF=y"
#endif
#if defined(CONFIG_LOG_BLOCK_IN_THREAD) && CONFIG_LOG_BLOCK_IN_THREAD
#error "trackball diagnostics require CONFIG_LOG_BLOCK_IN_THREAD=n"
#endif
#if !DT_NODE_EXISTS(DT_NODELABEL(trackball))
#error "trackball diagnostics require the trackball devicetree node"
#endif

#ifndef CHARYBDIS_DIAG_CONFIG_REV
#error "CHARYBDIS_DIAG_CONFIG_REV must be provided by the diagnostics CMake module"
#endif
#ifndef CHARYBDIS_DIAG_ZMK_REV
#error "CHARYBDIS_DIAG_ZMK_REV must be provided by the diagnostics CMake module"
#endif
#ifndef CHARYBDIS_DIAG_ZEPHYR_REV
#error "CHARYBDIS_DIAG_ZEPHYR_REV must be provided by the diagnostics CMake module"
#endif
#ifndef CHARYBDIS_DIAG_COMPILER
#error "CHARYBDIS_DIAG_COMPILER must be provided by the diagnostics CMake module"
#endif

LOG_MODULE_REGISTER(charybdis_trackball_diag, LOG_LEVEL_INF);

struct k_msgq;
struct zmk_hid_mouse_report_body;

extern struct k_msgq input_msgq;
extern struct k_msgq zmk_hog_mouse_msgq;

int input_report(const struct device *dev, uint8_t type, uint16_t code, int32_t value,
		 bool sync, k_timeout_t timeout);
int zmk_hog_send_mouse_report(struct zmk_hid_mouse_report_body *report);

int __real_input_report(const struct device *dev, uint8_t type, uint16_t code,
			int32_t value, bool sync, k_timeout_t timeout);
int __wrap_input_report(const struct device *dev, uint8_t type, uint16_t code,
			int32_t value, bool sync, k_timeout_t timeout);
int __real_zmk_hog_send_mouse_report(struct zmk_hid_mouse_report_body *report);
int __wrap_zmk_hog_send_mouse_report(struct zmk_hid_mouse_report_body *report);

static struct diag_stats stats;
static struct k_spinlock stats_lock;
static const struct device *const trackball_device = DEVICE_DT_GET(DT_NODELABEL(trackball));

static void sample_queues(void)
{
	uint32_t input_used = (uint32_t)k_msgq_num_used_get(&input_msgq);
	uint32_t input_capacity = (uint32_t)input_msgq.max_msgs;
	uint32_t hog_used = (uint32_t)k_msgq_num_used_get(&zmk_hog_mouse_msgq);
	uint32_t hog_capacity = (uint32_t)zmk_hog_mouse_msgq.max_msgs;
	k_spinlock_key_t key = k_spin_lock(&stats_lock);

	diag_stats_queue_sample(&stats, DIAG_QUEUE_INPUT, input_used, input_capacity);
	diag_stats_queue_sample(&stats, DIAG_QUEUE_HOG_MOUSE, hog_used, hog_capacity);
	k_spin_unlock(&stats_lock, key);
}

static void take_snapshot(struct diag_window_snapshot *window)
{
	k_spinlock_key_t key = k_spin_lock(&stats_lock);

	diag_stats_take_window(&stats, window);
	k_spin_unlock(&stats_lock, key);
}

int __wrap_input_report(const struct device *dev, uint8_t type, uint16_t code,
			int32_t value, bool sync, k_timeout_t timeout)
{
	uint64_t started_ms;
	uint64_t completed_ms;
	int result;
	k_spinlock_key_t key;

	if (dev != trackball_device) {
		return __real_input_report(dev, type, code, value, sync, timeout);
	}

	sample_queues();
	key = k_spin_lock(&stats_lock);
	diag_stats_begin(&stats, DIAG_OP_INPUT_REPORT);
	k_spin_unlock(&stats_lock, key);

	started_ms = (uint64_t)k_uptime_get();
	result = __real_input_report(dev, type, code, value, sync, timeout);
	completed_ms = (uint64_t)k_uptime_get();

	key = k_spin_lock(&stats_lock);
	diag_stats_complete(&stats, DIAG_OP_INPUT_REPORT, started_ms, completed_ms, result);
	k_spin_unlock(&stats_lock, key);
	sample_queues();
	return result;
}

int __wrap_zmk_hog_send_mouse_report(struct zmk_hid_mouse_report_body *report)
{
	uint64_t started_ms;
	uint64_t completed_ms;
	int result;
	k_spinlock_key_t key;

	sample_queues();
	key = k_spin_lock(&stats_lock);
	diag_stats_begin(&stats, DIAG_OP_HOG_MOUSE_REPORT);
	k_spin_unlock(&stats_lock, key);

	started_ms = (uint64_t)k_uptime_get();
	result = __real_zmk_hog_send_mouse_report(report);
	completed_ms = (uint64_t)k_uptime_get();

	key = k_spin_lock(&stats_lock);
	diag_stats_complete(&stats, DIAG_OP_HOG_MOUSE_REPORT, started_ms, completed_ms, result);
	k_spin_unlock(&stats_lock, key);
	sample_queues();
	return result;
}

static void diag_thread(void *unused1, void *unused2, void *unused3)
{
	int64_t window_started_ms = k_uptime_get();
	unsigned int logged_header = 0U;

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);
	while (true) {
		struct diag_window_snapshot window;
		int64_t now_ms;
		uint64_t elapsed_ms;

		k_sleep(K_SECONDS(1));
		sample_queues();
		now_ms = k_uptime_get();
		take_snapshot(&window);
		elapsed_ms = (uint64_t)now_ms - (uint64_t)window_started_ms;
		window_started_ms = now_ms;

		if (logged_header == 0U) {
			LOG_INF("probe=charybdis_trackball_diag scope=input(trackball-only),hog(all-mouse-reports) config_rev=%s zmk_rev=%s zephyr_rev=%s type=%s compiler=%s uptime_unit=ms tick_hz=%u input_mode_thread=%u studio=%u ble=%u studio_ble=%u usb_hid=%u queues(input=%u hog=%u)",
				CHARYBDIS_DIAG_CONFIG_REV, CHARYBDIS_DIAG_ZMK_REV,
				CHARYBDIS_DIAG_ZEPHYR_REV, CHARYBDIS_DIAG_BUILD_TYPE,
				CHARYBDIS_DIAG_COMPILER,
				(unsigned int)CONFIG_SYS_CLOCK_TICKS_PER_SEC,
				(unsigned int)IS_ENABLED(CONFIG_INPUT_MODE_THREAD),
				(unsigned int)IS_ENABLED(CONFIG_ZMK_STUDIO),
				(unsigned int)IS_ENABLED(CONFIG_ZMK_BLE),
				(unsigned int)IS_ENABLED(CONFIG_ZMK_STUDIO_TRANSPORT_BLE),
				(unsigned int)IS_ENABLED(CONFIG_ZMK_USB),
				(unsigned int)input_msgq.max_msgs,
				(unsigned int)zmk_hog_mouse_msgq.max_msgs);
			logged_header = 1U;
		}

		LOG_INF("input window_ms=%llu uptime_ms=%llu begun=%llu done=%llu errors=%llu buckets_ms[<1,<5,<20,<100,>=100]=%llu,%llu,%llu,%llu,%llu max_ms=%llu max_done_ms=%llu last_done_ms=%llu totals_begun/done/errors=%llu/%llu/%llu",
			(unsigned long long)elapsed_ms, (unsigned long long)now_ms,
			(unsigned long long)window.operations[DIAG_OP_INPUT_REPORT].window_begun,
			(unsigned long long)window.operations[DIAG_OP_INPUT_REPORT].window_completed,
			(unsigned long long)window.operations[DIAG_OP_INPUT_REPORT].window_errors,
			(unsigned long long)window.operations[DIAG_OP_INPUT_REPORT].window_buckets[0],
			(unsigned long long)window.operations[DIAG_OP_INPUT_REPORT].window_buckets[1],
			(unsigned long long)window.operations[DIAG_OP_INPUT_REPORT].window_buckets[2],
			(unsigned long long)window.operations[DIAG_OP_INPUT_REPORT].window_buckets[3],
			(unsigned long long)window.operations[DIAG_OP_INPUT_REPORT].window_buckets[4],
			(unsigned long long)window.operations[DIAG_OP_INPUT_REPORT].window_max_ms,
			(unsigned long long)window.operations[DIAG_OP_INPUT_REPORT].window_max_done_uptime_ms,
			(unsigned long long)window.operations[DIAG_OP_INPUT_REPORT].last_complete_uptime_ms,
			(unsigned long long)window.operations[DIAG_OP_INPUT_REPORT].total_begun,
			(unsigned long long)window.operations[DIAG_OP_INPUT_REPORT].total_completed,
			(unsigned long long)window.operations[DIAG_OP_INPUT_REPORT].total_errors);
		LOG_INF("hog window_ms=%llu uptime_ms=%llu begun=%llu done=%llu errors=%llu buckets_ms[<1,<5,<20,<100,>=100]=%llu,%llu,%llu,%llu,%llu max_ms=%llu max_done_ms=%llu last_done_ms=%llu totals_begun/done/errors=%llu/%llu/%llu",
			(unsigned long long)elapsed_ms, (unsigned long long)now_ms,
			(unsigned long long)window.operations[DIAG_OP_HOG_MOUSE_REPORT].window_begun,
			(unsigned long long)window.operations[DIAG_OP_HOG_MOUSE_REPORT].window_completed,
			(unsigned long long)window.operations[DIAG_OP_HOG_MOUSE_REPORT].window_errors,
			(unsigned long long)window.operations[DIAG_OP_HOG_MOUSE_REPORT].window_buckets[0],
			(unsigned long long)window.operations[DIAG_OP_HOG_MOUSE_REPORT].window_buckets[1],
			(unsigned long long)window.operations[DIAG_OP_HOG_MOUSE_REPORT].window_buckets[2],
			(unsigned long long)window.operations[DIAG_OP_HOG_MOUSE_REPORT].window_buckets[3],
			(unsigned long long)window.operations[DIAG_OP_HOG_MOUSE_REPORT].window_buckets[4],
			(unsigned long long)window.operations[DIAG_OP_HOG_MOUSE_REPORT].window_max_ms,
			(unsigned long long)window.operations[DIAG_OP_HOG_MOUSE_REPORT].window_max_done_uptime_ms,
			(unsigned long long)window.operations[DIAG_OP_HOG_MOUSE_REPORT].last_complete_uptime_ms,
			(unsigned long long)window.operations[DIAG_OP_HOG_MOUSE_REPORT].total_begun,
			(unsigned long long)window.operations[DIAG_OP_HOG_MOUSE_REPORT].total_completed,
			(unsigned long long)window.operations[DIAG_OP_HOG_MOUSE_REPORT].total_errors);
		LOG_INF("queue samples/input=%llu full_samples=%llu sampled_peak=%u capacity=%u; hog=%llu full_samples=%llu sampled_peak=%u capacity=%u (sampled high-water marks, not exact peaks)",
			(unsigned long long)window.queues[DIAG_QUEUE_INPUT].window_samples,
			(unsigned long long)window.queues[DIAG_QUEUE_INPUT].window_full_samples,
			(unsigned int)window.queues[DIAG_QUEUE_INPUT].window_peak_used,
			(unsigned int)window.queues[DIAG_QUEUE_INPUT].capacity,
			(unsigned long long)window.queues[DIAG_QUEUE_HOG_MOUSE].window_samples,
			(unsigned long long)window.queues[DIAG_QUEUE_HOG_MOUSE].window_full_samples,
			(unsigned int)window.queues[DIAG_QUEUE_HOG_MOUSE].window_peak_used,
			(unsigned int)window.queues[DIAG_QUEUE_HOG_MOUSE].capacity);
	}
}

#define DIAG_THREAD_STACK_SIZE 1536
K_THREAD_DEFINE(charybdis_diag_tid, DIAG_THREAD_STACK_SIZE, diag_thread,
		NULL, NULL, NULL, K_PRIO_PREEMPT(CONFIG_NUM_PREEMPT_PRIORITIES - 1), 0, 0);
