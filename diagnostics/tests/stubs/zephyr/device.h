#ifndef TEST_ZEPHYR_DEVICE_H
#define TEST_ZEPHYR_DEVICE_H

struct device {
	int identifier;
};

extern const struct device test_trackball;

#define DEVICE_DT_GET(node) (&test_trackball)

#endif
