#include "fixture.h"

int __real_input_report(int value);
int __real_zmk_hog_send_mouse_report(struct zmk_hid_mouse_report_body *report);

unsigned int wrapped_input_calls;
unsigned int wrapped_hog_calls;
int forwarded_input_value;
struct zmk_hid_mouse_report_body *forwarded_hog_report;

__attribute__((noinline)) int __wrap_input_report(int value)
{
	wrapped_input_calls++;
	forwarded_input_value = value;
	return __real_input_report(value);
}

__attribute__((noinline)) int __wrap_zmk_hog_send_mouse_report(
	struct zmk_hid_mouse_report_body *report)
{
	wrapped_hog_calls++;
	forwarded_hog_report = report;
	return __real_zmk_hog_send_mouse_report(report);
}
