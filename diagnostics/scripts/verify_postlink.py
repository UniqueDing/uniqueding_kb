#!/usr/bin/env python3
"""Fail-closed checks for the final diagnostic firmware ELF."""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path
from typing import Iterable


class VerificationError(RuntimeError):
    pass


FUNCTION_HEADER = re.compile(r"^\s*[0-9a-fA-F]+\s+<(.+)>:\s*$")
SYMBOL_LINE = re.compile(r"^\s*([0-9a-fA-F]+)\s+\S\s+(.+?)\s*$")
INSTRUCTION_LINE = re.compile(
    r"^\s*(?P<address>[0-9a-fA-F]+):\s+"
    r"(?:(?:[0-9a-fA-F]{2,16})\s+)*(?P<mnemonic>[A-Za-z][A-Za-z0-9.]*)"
    r"\s*(?P<operands>.*)$"
)
RELOCATION_LINE = re.compile(
    r"^\s*(?P<address>[0-9a-fA-F]+):\s+R_[A-Za-z0-9_]+\s+(?P<symbol>[^\s+]+)"
)
DIRECT_BRANCH_MNEMONICS = {
    "bl", "blx", "b.w", "b.n", "b", "call", "callq", "jmp", "jmpq"
}


def parse_symbols(nm_output: str) -> dict[str, str]:
    symbols: dict[str, str] = {}
    for line in nm_output.splitlines():
        match = SYMBOL_LINE.match(line)
        if match:
            symbols[match.group(2)] = match.group(1).lower()
    return symbols


def direct_target_matches(operands: str, symbol: str) -> bool:
    annotation = re.search(r"<([^<>]+)>", operands)
    if annotation:
        target = annotation.group(1).split("+", 1)[0].strip()
        return target == symbol
    first_operand = operands.strip().split(",", 1)[0].split()
    if not first_operand:
        return False
    target = first_operand[0].lstrip("*").split("+", 1)[0]
    return target == symbol


def direct_branch_sites(disassembly: str) -> list[tuple[str, str, str, str]]:
    """Return (caller, mnemonic, operands, relocation_target) direct branches only."""
    sites: list[tuple[str, str, str, str]] = []
    current_function: str | None = None
    previous_instruction: tuple[str, str, str, str] | None = None

    for line in disassembly.splitlines():
        header = FUNCTION_HEADER.match(line)
        if header:
            current_function = header.group(1)
            previous_instruction = None
            continue

        if current_function is None:
            continue

        relocation = RELOCATION_LINE.match(line)
        if relocation:
            if previous_instruction is not None:
                address, mnemonic, operands, caller = previous_instruction
                if (address == relocation.group("address").lower() and
                        mnemonic in DIRECT_BRANCH_MNEMONICS):
                    sites.append((caller, mnemonic, operands, relocation.group("symbol")))
            continue

        instruction = INSTRUCTION_LINE.match(line)
        if instruction:
            mnemonic = instruction.group("mnemonic").lower()
            previous_instruction = (
                instruction.group("address").lower(), mnemonic,
                instruction.group("operands"), current_function
            )
            if mnemonic in DIRECT_BRANCH_MNEMONICS:
                sites.append((current_function, mnemonic, instruction.group("operands"), ""))
            continue

        previous_instruction = None

    return sites


def callers_of(disassembly: str, symbol: str) -> list[str]:
    callers = []
    for caller, _mnemonic, operands, relocation_target in direct_branch_sites(disassembly):
        if direct_target_matches(operands, symbol) or relocation_target == symbol:
            callers.append(caller)
    return callers


def function_directly_calls(disassembly: str, function: str, symbol: str) -> bool:
    return function in callers_of(disassembly, symbol)


def verify_outputs(nm_output: str, disassembly: str) -> None:
    symbols = parse_symbols(nm_output)
    required_symbols = (
        "__wrap_input_report",
        "__wrap_zmk_hog_send_mouse_report",
        "input_report",
        "zmk_hog_send_mouse_report",
        "input_msgq",
        "zmk_hog_mouse_msgq",
    )
    missing = [symbol for symbol in required_symbols if symbol not in symbols]
    if missing:
        raise VerificationError(f"final ELF is missing required symbols: {', '.join(missing)}")

    for wrapper, real in (
        ("__wrap_input_report", "input_report"),
        ("__wrap_zmk_hog_send_mouse_report", "zmk_hog_send_mouse_report"),
    ):
        if symbols[wrapper] == symbols[real]:
            raise VerificationError(f"wrapper and real callee alias at the same address: {wrapper}/{real}")

    for wrapper, real in (
        ("__wrap_input_report", "input_report"),
        ("__wrap_zmk_hog_send_mouse_report", "zmk_hog_send_mouse_report"),
    ):
        if not function_directly_calls(disassembly, wrapper, real):
            raise VerificationError(f"{wrapper} has no direct call/branch to its distinct real callee {real}")

    input_callers = [name for name in callers_of(disassembly, "__wrap_input_report")
                     if name != "__wrap_input_report"]
    pmw_callers = [name for name in input_callers if re.search(r"pmw3610|pixart|trackball", name, re.I)]
    if not pmw_callers:
        raise VerificationError(
            "no PMW3610/trackball function in the final ELF calls __wrap_input_report"
        )

    hog_callers = [name for name in callers_of(disassembly, "__wrap_zmk_hog_send_mouse_report")
                   if name != "__wrap_zmk_hog_send_mouse_report"]
    endpoint_callers = [
        name for name in hog_callers
        if re.search(r"endpoint|mouse_report|send_mouse|hid_mouse", name, re.I)
    ]
    if not endpoint_callers:
        raise VerificationError(
            "no endpoint/mouse-report producer function in the final ELF calls "
            "__wrap_zmk_hog_send_mouse_report"
        )


def run_tool(command: Iterable[str]) -> str:
    result = subprocess.run(list(command), check=False, text=True, capture_output=True)
    if result.returncode != 0:
        raise VerificationError(
            f"tool failed ({result.returncode}): {' '.join(command)}\n{result.stderr.strip()}"
        )
    return result.stdout


def verify_elf(elf: Path, nm: str, objdump: str) -> None:
    if not elf.is_file():
        raise VerificationError(f"final ELF does not exist: {elf}")
    nm_output = run_tool((nm, "-n", str(elf)))
    disassembly = run_tool((objdump, "-drC", str(elf)))
    verify_outputs(nm_output, disassembly)
    print(f"PASS: verified diagnostic wrapper call paths and queue symbols in {elf}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--elf", required=True, type=Path)
    parser.add_argument("--nm", required=True)
    parser.add_argument("--objdump", required=True)
    args = parser.parse_args()
    try:
        verify_elf(args.elf, args.nm, args.objdump)
    except VerificationError as exc:
        print(f"ERROR: diagnostic post-link verification failed: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
