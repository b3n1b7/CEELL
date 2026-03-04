#!/usr/bin/env python3
"""
CEELL Code Coverage Threshold Check

Parses a Cobertura XML coverage report (as produced by gcovr) and checks
whether line coverage meets a configurable threshold. Exits with code 1
if coverage is below the threshold.

Usage:
    python3 tools/check_coverage.py [--threshold PERCENT] <coverage.xml>

Examples:
    python3 tools/check_coverage.py twister-out/coverage.xml
    python3 tools/check_coverage.py --threshold 80 twister-out/coverage.xml
"""

import argparse
import sys
import xml.etree.ElementTree as ET


def parse_cobertura(xml_path):
    """Parse a Cobertura XML file and return (lines_valid, lines_covered, line_rate).

    The Cobertura format stores aggregate stats on the root <coverage> element:
        lines-valid   = total executable lines
        lines-covered = lines hit at least once
        line-rate     = lines-covered / lines-valid (0.0 .. 1.0)

    Returns:
        tuple: (lines_valid, lines_covered, line_rate_pct)
    """
    tree = ET.parse(xml_path)
    root = tree.getroot()

    lines_valid = int(root.attrib.get("lines-valid", 0))
    lines_covered = int(root.attrib.get("lines-covered", 0))
    line_rate = float(root.attrib.get("line-rate", 0.0))

    # Convert to percentage
    line_rate_pct = line_rate * 100.0

    return lines_valid, lines_covered, line_rate_pct


def print_per_file_summary(xml_path):
    """Print per-file coverage breakdown from a Cobertura XML."""
    tree = ET.parse(xml_path)
    root = tree.getroot()

    print("\n  Per-file coverage:")
    print("  {:<60s} {:>8s}".format("File", "Line %"))
    print("  " + "-" * 70)

    for package in root.findall(".//package"):
        for cls in package.findall(".//class"):
            filename = cls.attrib.get("filename", "?")
            file_rate = float(cls.attrib.get("line-rate", 0.0)) * 100.0
            print("  {:<60s} {:>7.1f}%".format(filename, file_rate))


def main():
    parser = argparse.ArgumentParser(
        description="Check code coverage against a threshold"
    )
    parser.add_argument(
        "coverage_xml",
        help="Path to Cobertura XML coverage report",
    )
    parser.add_argument(
        "--threshold",
        type=float,
        default=60.0,
        help="Minimum line coverage percentage (default: 60)",
    )
    args = parser.parse_args()

    try:
        lines_valid, lines_covered, line_rate_pct = parse_cobertura(
            args.coverage_xml
        )
    except FileNotFoundError:
        print(f"ERROR: Coverage file not found: {args.coverage_xml}")
        sys.exit(1)
    except ET.ParseError as e:
        print(f"ERROR: Failed to parse XML: {e}")
        sys.exit(1)

    print("=" * 60)
    print("  CEELL Code Coverage Summary")
    print("=" * 60)
    print(f"  Total lines:    {lines_valid}")
    print(f"  Covered lines:  {lines_covered}")
    print(f"  Line coverage:  {line_rate_pct:.1f}%")
    print(f"  Threshold:      {args.threshold:.1f}%")

    try:
        print_per_file_summary(args.coverage_xml)
    except Exception:
        pass  # Per-file summary is best-effort

    print()

    if line_rate_pct < args.threshold:
        print(
            f"  FAIL: Line coverage {line_rate_pct:.1f}% is below "
            f"threshold {args.threshold:.1f}%"
        )
        print("=" * 60)
        sys.exit(1)
    else:
        print(
            f"  PASS: Line coverage {line_rate_pct:.1f}% meets "
            f"threshold {args.threshold:.1f}%"
        )
        print("=" * 60)
        sys.exit(0)


if __name__ == "__main__":
    main()
