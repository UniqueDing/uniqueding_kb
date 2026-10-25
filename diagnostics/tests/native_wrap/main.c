#include <assert.h>

#include "fixture.h"

int main(void)
{
	struct zmk_hid_mouse_report_body report = {.marker = 23};

	assert(pmw3610_process(17) == 117);
	assert(wrapped_input_calls == 1U);
	assert(forwarded_input_value == 17);
	assert(zmk_endpoints_send_mouse_report(&report) == 223);
	assert(wrapped_hog_calls == 1U);
	assert(forwarded_hog_report == &report);
	return 0;
}
