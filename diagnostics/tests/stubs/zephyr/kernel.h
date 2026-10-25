#ifndef TEST_ZEPHYR_KERNEL_H
#define TEST_ZEPHYR_KERNEL_H

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/autoconf.h>

typedef int64_t k_timeout_t;
typedef int k_spinlock_key_t;
typedef uint8_t k_thread_stack_t;

struct k_msgq {
	uint32_t max_msgs;
	uint32_t used;
};

struct k_spinlock {
	int unused;
};

extern pthread_mutex_t test_spinlock_mutex;
extern _Thread_local int test_spinlock_depth;

static inline k_spinlock_key_t k_spin_lock(struct k_spinlock *lock)
{
	(void)lock;
	assert(test_spinlock_depth == 0);
	assert(pthread_mutex_lock(&test_spinlock_mutex) == 0);
	test_spinlock_depth++;
	return 0;
}

static inline void k_spin_unlock(struct k_spinlock *lock, k_spinlock_key_t key)
{
	(void)lock;
	(void)key;
	assert(test_spinlock_depth == 1);
	test_spinlock_depth--;
	assert(pthread_mutex_unlock(&test_spinlock_mutex) == 0);
}

static inline uint32_t k_msgq_num_used_get(struct k_msgq *msgq)
{
	return msgq->used;
}

int64_t k_uptime_get(void);
static inline void k_sleep(k_timeout_t timeout) { (void)timeout; }

#define K_SECONDS(seconds) ((k_timeout_t)(seconds) * 1000)
#define K_PRIO_PREEMPT(priority) (priority)
#define K_THREAD_STACK_DEFINE(name, size) static uint8_t name[size] __attribute__((unused))
#define K_THREAD_DEFINE(name, stack_size, entry, a1, a2, a3, priority, options, delay) \
	static void (*name)(void *, void *, void *) __attribute__((unused)) = entry
#define ARG_UNUSED(value) ((void)(value))

#endif
