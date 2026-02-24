#!/usr/bin/env python3
"""Generate a flash-ready binary from a CEELL JSON config file.

Reads a JSON config, validates it, and writes a binary padded with 0xFF
to fill the 64KB config partition. This matches erased NOR flash state.

Usage:
    python3 tools/gen_config_bin.py config/node_body.json config/body_config.bin
"""

import json
import sys

PARTITION_SIZE = 64 * 1024  # 64KB config partition
REQUIRED_FIELDS = {"version", "node_id", "role", "name", "ip", "netmask"}
OPTIONAL_FIELDS = {"services"}
SUPPORTED_VERSIONS = {1}


def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <input.json> <output.bin>", file=sys.stderr)
        sys.exit(1)

    input_path = sys.argv[1]
    output_path = sys.argv[2]

    with open(input_path, "r") as f:
        config = json.load(f)

    # Validate required fields
    missing = REQUIRED_FIELDS - set(config.keys())
    if missing:
        print(f"Error: missing fields: {missing}", file=sys.stderr)
        sys.exit(1)

    # Validate version
    if config["version"] not in SUPPORTED_VERSIONS:
        print(f"Error: unsupported version {config['version']}, "
              f"supported: {SUPPORTED_VERSIONS}", file=sys.stderr)
        sys.exit(1)

    # Compact JSON (no whitespace)
    json_bytes = json.dumps(config, separators=(",", ":")).encode("utf-8")

    if len(json_bytes) >= PARTITION_SIZE:
        print(f"Error: JSON ({len(json_bytes)} bytes) exceeds "
              f"partition size ({PARTITION_SIZE} bytes)", file=sys.stderr)
        sys.exit(1)

    # Pad with 0xFF (erased NOR flash state)
    binary = json_bytes + b"\xff" * (PARTITION_SIZE - len(json_bytes))

    with open(output_path, "wb") as f:
        f.write(binary)

    print(f"Generated {output_path}: {len(json_bytes)} bytes JSON + "
          f"{PARTITION_SIZE - len(json_bytes)} bytes padding = {PARTITION_SIZE} bytes total")


if __name__ == "__main__":
    main()
