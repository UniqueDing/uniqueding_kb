#ifndef DIAGNOSTIC_WRAP_FIXTURE_H
#define DIAGNOSTIC_WRAP_FIXTURE_H

struct zmk_hid_mouse_report_body {
	int marker;
};

extern volatile unsigned int input_msgq[2];
extern volatile unsigned int zmk_hog_mouse_msgq[2];

int input_report(int value);
int zmk_hog_send_mouse_report(struct zmk_hid_mouse_report_body *report);
int pmw3610_process(int value);
int zmk_endpoints_send_mouse_report(struct zmk_hid_mouse_report_body *report);

extern unsigned int wrapped_input_calls;
extern unsigned int wrapped_hog_calls;
extern int forwarded_input_value;
extern struct zmk_hid_mouse_report_body *forwarded_hog_report;

#endif
