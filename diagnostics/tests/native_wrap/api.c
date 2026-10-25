#include "fixture.h"

volatile unsigned int input_msgq[2];
volatile unsigned int zmk_hog_mouse_msgq[2];

__attribute__((noinline)) int input_report(int value)
{
	return value + 100;
}

__attribute__((noinline)) int zmk_hog_send_mouse_report(struct zmk_hid_mouse_report_body *report)
{
	return report->marker + 200;
}
