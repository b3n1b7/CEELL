#!/usr/bin/env python3
"""
CEELL Requirements Traceability Checker

Scans C source files in ceell/ and src/ for /* REQ-xxx-yyy */ comments,
and scans test files in tests/ for the same pattern. Reports which
requirements have source implementations, which have test coverage,
and which have gaps.

This tool is advisory — it always exits 0.

Usage:
    python3 tools/check_traceability.py [--project-root DIR]
"""

import argparse
import os
import re
import sys
from collections import defaultdict

# Pattern matches requirement tags like: /* REQ-OSAL-001 */
# Also matches when embedded in a line: ... /* REQ-MSG-003 */ ...
# And multi-tag lines: /* REQ-OSAL-001, REQ-OSAL-002 */
REQ_PATTERN = re.compile(r"REQ-[A-Z]+-\d{3}")

# All known requirement IDs
KNOWN_REQUIREMENTS = [
    "REQ-OSAL-001",
    "REQ-OSAL-002",
    "REQ-OSAL-003",
    "REQ-OSAL-004",
    "REQ-OSAL-005",
    "REQ-MSG-001",
    "REQ-MSG-002",
    "REQ-MSG-003",
    "REQ-MSG-004",
    "REQ-MSG-005",
    "REQ-SAF-001",
    "REQ-SAF-002",
    "REQ-SAF-003",
    "REQ-SAF-004",
    "REQ-SAF-005",
    "REQ-NET-001",
    "REQ-NET-002",
    "REQ-NET-003",
    "REQ-OTA-001",
    "REQ-OTA-002",
    "REQ-OTA-003",
]


def scan_directory(root_dir, subdirs, extensions=(".c", ".h")):
    """Scan files in the given subdirectories for requirement tags.

    Args:
        root_dir: Project root directory
        subdirs: List of subdirectory paths relative to root_dir
        extensions: File extensions to scan

    Returns:
        dict: {req_id: [(filepath, line_number, line_text), ...]}
    """
    findings = defaultdict(list)

    for subdir in subdirs:
        scan_path = os.path.join(root_dir, subdir)
        if not os.path.isdir(scan_path):
            continue

        for dirpath, _, filenames in os.walk(scan_path):
            for filename in filenames:
                if not any(filename.endswith(ext) for ext in extensions):
                    continue

                filepath = os.path.join(dirpath, filename)
                rel_path = os.path.relpath(filepath, root_dir)

                try:
                    with open(filepath, "r", encoding="utf-8", errors="replace") as f:
                        for line_num, line in enumerate(f, 1):
                            matches = REQ_PATTERN.findall(line)
                            for req_id in matches:
                                findings[req_id].append(
                                    (rel_path, line_num, line.strip())
                                )
                except OSError:
                    continue

    return findings


def main():
    parser = argparse.ArgumentParser(
        description="Check requirements traceability in CEELL source and test files"
    )
    parser.add_argument(
        "--project-root",
        default=os.path.join(os.path.dirname(__file__), ".."),
        help="Project root directory (default: parent of tools/)",
    )
    args = parser.parse_args()

    root = os.path.abspath(args.project_root)

    print("=" * 70)
    print("  CEELL Requirements Traceability Report")
    print("=" * 70)
    print(f"  Project root: {root}")
    print()

    # Scan source directories
    source_dirs = ["ceell/core", "ceell/network", "ceell/ota", "ceell/osal", "src"]
    source_findings = scan_directory(root, source_dirs)

    # Scan test directories (include .robot files as well as .c/.h)
    test_dirs = ["tests"]
    test_findings = scan_directory(
        root, test_dirs, extensions=(".c", ".h", ".robot")
    )

    # Also scan shell scripts and python tools (they may contain req refs)
    tool_dirs = ["tools"]
    tool_findings = scan_directory(
        root, tool_dirs, extensions=(".py", ".sh")
    )

    # Merge tool findings into source findings
    for req_id, refs in tool_findings.items():
        source_findings[req_id].extend(refs)

    # Report
    print("  {:<16s} {:<12s} {:<12s} {:s}".format(
        "Requirement", "Source", "Test", "Status"
    ))
    print("  " + "-" * 60)

    has_source_count = 0
    has_test_count = 0
    gap_count = 0

    for req_id in KNOWN_REQUIREMENTS:
        has_source = req_id in source_findings
        has_test = req_id in test_findings

        if has_source:
            has_source_count += 1
        if has_test:
            has_test_count += 1

        if has_source and has_test:
            status = "TRACED"
        elif has_source and not has_test:
            status = "NO TEST"
            gap_count += 1
        elif not has_source and has_test:
            status = "NO SOURCE"
            gap_count += 1
        else:
            status = "MISSING"
            gap_count += 1

        src_marker = "yes" if has_source else "---"
        tst_marker = "yes" if has_test else "---"

        print("  {:<16s} {:<12s} {:<12s} {:s}".format(
            req_id, src_marker, tst_marker, status
        ))

    total = len(KNOWN_REQUIREMENTS)
    print()
    print(f"  Total requirements: {total}")
    print(f"  With source refs:   {has_source_count}")
    print(f"  With test refs:     {has_test_count}")
    print(f"  Gaps:               {gap_count}")

    # Show unknown requirement refs found in code
    all_found = set(source_findings.keys()) | set(test_findings.keys())
    unknown = all_found - set(KNOWN_REQUIREMENTS)
    if unknown:
        print()
        print("  WARNING: Unknown requirement IDs found in code:")
        for req_id in sorted(unknown):
            locations = source_findings.get(req_id, []) + test_findings.get(req_id, [])
            for rel_path, line_num, _ in locations:
                print(f"    {req_id} at {rel_path}:{line_num}")

    # Detail section
    if source_findings or test_findings:
        print()
        print("  Detailed references:")
        print()
        for req_id in KNOWN_REQUIREMENTS:
            refs = source_findings.get(req_id, []) + test_findings.get(req_id, [])
            if refs:
                print(f"  {req_id}:")
                for rel_path, line_num, line_text in refs:
                    # Truncate long lines
                    display = line_text[:72] + "..." if len(line_text) > 72 else line_text
                    print(f"    {rel_path}:{line_num}: {display}")
                print()

    print("=" * 70)
    print("  NOTE: This check is advisory. Exit code is always 0.")
    print("=" * 70)

    # Advisory only — always exit 0
    sys.exit(0)


if __name__ == "__main__":
    main()
