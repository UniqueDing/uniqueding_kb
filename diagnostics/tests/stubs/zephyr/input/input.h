#ifndef TEST_ZEPHYR_INPUT_INPUT_H
#define TEST_ZEPHYR_INPUT_INPUT_H

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>

int input_report(const struct device *dev, uint8_t type, uint16_t code, int32_t value,
		 bool sync, k_timeout_t timeout);

#endif
