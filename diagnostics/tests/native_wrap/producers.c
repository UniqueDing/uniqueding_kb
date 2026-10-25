#include "fixture.h"

__attribute__((noinline)) int pmw3610_process(int value)
{
	return input_report(value);
}

__attribute__((noinline)) int zmk_endpoints_send_mouse_report(
	struct zmk_hid_mouse_report_body *report)
{
	return zmk_hog_send_mouse_report(report);
}
