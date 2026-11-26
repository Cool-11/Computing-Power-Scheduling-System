"""Write reproducible .in files for the tpj_win.exe local judge.

Usage: python tests/generate_local_worlds.py C:/path/to/empty-test-directory
The script only writes its named .in files; it does not remove existing files.
"""

import argparse
from pathlib import Path


BASE = Path(__file__).with_name("local_world_base.in").read_text(encoding="utf-8")
VARIANTS = {
    "low_load": (20261221, {"load": "0.55"}),
    "high_load": (20261222, {"load": "1.25"}),
    "high_variance": (20261223, {"sigma": "0.8", "bias_lo": "0.5", "bias_hi": "2.5"}),
    "fail_heavy": (20261224, {"fail_frac": "0.3", "precursor_frac": "0.5"}),
    "network_heavy": (20261225, {"link_events": "500"}),
    "log_heavy": (20261226, {"log_rate": "0.35", "audit_rate": "5.0"}),
    "cheap_values": (20261227, {"p_noise": "2000"}),
    "no_blackswan": (20261228, {"blackswan": "0"}),
}


def render(changes):
    result = []
    for line in BASE.splitlines():
        key, value = line.split(maxsplit=1)
        result.append(f"{key} {changes.get(key, value)}")
    return "\n".join(result) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output_dir", type=Path)
    args = parser.parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    for seed in range(20261105, 20261121):
        (args.output_dir / f"base_{seed}.in").write_text(
            render({"seed": str(seed)}), encoding="ascii"
        )
    for name, (seed, changes) in VARIANTS.items():
        (args.output_dir / f"{name}.in").write_text(
            render({"seed": str(seed), **changes}), encoding="ascii"
        )
    print(f"wrote 24 worlds to {args.output_dir}")


if __name__ == "__main__":
    main()
