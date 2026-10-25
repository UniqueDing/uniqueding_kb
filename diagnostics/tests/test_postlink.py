import unittest
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from scripts.verify_postlink import VerificationError, verify_outputs


NM_GOOD = """\
00000010 T __wrap_input_report
00000020 T input_report
00000030 T __wrap_zmk_hog_send_mouse_report
00000040 T zmk_hog_send_mouse_report
00000050 B input_msgq
00000060 B zmk_hog_mouse_msgq
"""

DISASM_GOOD = """\
00000010 <__wrap_input_report>:
  10: f000 f800 bl 20 <input_report>

00000020 <input_report>:
  20: bx lr

00000100 <pmw3610_process>:
  100: f000 f800 bl 10 <__wrap_input_report>
  100: R_ARM_THM_CALL __wrap_input_report

00000030 <__wrap_zmk_hog_send_mouse_report>:
  30: e8 00 00 00 00 callq 40 <zmk_hog_send_mouse_report>

00000040 <zmk_hog_send_mouse_report>:
  40: bx lr

00000120 <zmk_endpoints_send_mouse_report>:
  120: 94000000 b 30 <__wrap_zmk_hog_send_mouse_report>
"""


class PostlinkTests(unittest.TestCase):
    def test_accepts_real_wrapper_paths_and_queues(self):
        verify_outputs(NM_GOOD, DISASM_GOOD)

    def test_rejects_missing_input_producer_path(self):
        no_pmw = DISASM_GOOD.replace("bl 10 <__wrap_input_report>", "bl 10 <some_other_function>")
        no_pmw = no_pmw.replace("R_ARM_THM_CALL __wrap_input_report", "R_ARM_THM_CALL some_other_function")
        with self.assertRaises(VerificationError):
            verify_outputs(NM_GOOD, no_pmw)

    def test_accepts_arm_direct_call_and_thumb_tail_call_forms(self):
        for instruction in (
            "f000 f800 bl 10 <__wrap_input_report>",
            "f000 e800 blx 10 <__wrap_input_report>",
            "f000 9000 b.w 10 <__wrap_input_report>",
        ):
            with self.subTest(instruction=instruction):
                fixture = DISASM_GOOD.replace(
                    "100: f000 f800 bl 10 <__wrap_input_report>",
                    f"100: {instruction}",
                )
                verify_outputs(NM_GOOD, fixture)

    def test_accepts_relocation_for_arm_branch_target(self):
        fixture = DISASM_GOOD.replace(
            "bl 10 <__wrap_input_report>", "bl 0x0"
        )
        verify_outputs(NM_GOOD, fixture)

    def test_accepts_host_direct_jump(self):
        fixture = DISASM_GOOD.replace(
            "120: 94000000 b 30 <__wrap_zmk_hog_send_mouse_report>",
            "120: e9 00 00 00 00 jmpq 30 <__wrap_zmk_hog_send_mouse_report>",
        )
        verify_outputs(NM_GOOD, fixture)

    def test_rejects_data_or_comment_symbol_references(self):
        non_call = DISASM_GOOD.replace(
            "100: f000 f800 bl 10 <__wrap_input_report>\n"
            "  100: R_ARM_THM_CALL __wrap_input_report",
            "100: .word __wrap_input_report  ; not a branch instruction",
        )
        with self.assertRaises(VerificationError):
            verify_outputs(NM_GOOD, non_call)

    def test_rejects_symbol_mentioned_only_in_branch_comment(self):
        comment_only = DISASM_GOOD.replace(
            "bl 10 <__wrap_input_report>\n  100: R_ARM_THM_CALL __wrap_input_report",
            "bl 10 <some_other_function> ; __wrap_input_report",
        )
        with self.assertRaises(VerificationError):
            verify_outputs(NM_GOOD, comment_only)

    def test_rejects_wrapper_recursion_alias(self):
        same_address = NM_GOOD.replace("00000020 T input_report", "00000010 T input_report")
        with self.assertRaises(VerificationError):
            verify_outputs(same_address, DISASM_GOOD)

    def test_rejects_missing_hog_queue(self):
        no_queue = NM_GOOD.replace("00000060 B zmk_hog_mouse_msgq\n", "")
        with self.assertRaises(VerificationError):
            verify_outputs(no_queue, DISASM_GOOD)

    def test_rejects_missing_endpoint_to_hog_path(self):
        no_endpoint = DISASM_GOOD.replace("zmk_endpoints_send_mouse_report", "some_unrelated_function")
        with self.assertRaises(VerificationError):
            verify_outputs(NM_GOOD, no_endpoint)


if __name__ == "__main__":
    unittest.main()
