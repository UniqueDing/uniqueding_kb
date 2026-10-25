#ifndef TEST_ZEPHYR_LOGGING_LOG_H
#define TEST_ZEPHYR_LOGGING_LOG_H

#include <stdarg.h>

static inline int test_log(const char *format, ...)
	__attribute__((format(printf, 1, 2)));

static inline int test_log(const char *format, ...)
{
	va_list args;
	(void)format;
	va_start(args, format);
	va_end(args);
	return 0;
}

#define LOG_LEVEL_INF 3
#define LOG_MODULE_REGISTER(name, level) static const int log_module_##name __attribute__((unused)) = level
#define LOG_INF(...) ((void)test_log(__VA_ARGS__))

#endif
