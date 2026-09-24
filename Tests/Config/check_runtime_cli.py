#!/usr/bin/env python3
"""Exercise host configuration loading and precedence without STM32 hardware."""

import argparse
import json
import math
from pathlib import Path
import shutil
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", required=True, type=Path)
    parser.add_argument("--config-dir", type=Path, default=Path("config"))
    args = parser.parse_args()
    binaries = {name: args.build_dir.resolve() / name for name in
                ("sil_runner", "hil_runner", "sil_monte_carlo")}
    checks = 0

    with tempfile.TemporaryDirectory(prefix="drone-config-cli-") as temporary:
        root = Path(temporary)
        shutil.copytree(args.config_dir, root / "config")
        (root / "sil-test.conf").write_text("viewer_enabled = false\n")

        def run(name, arguments, expected=(0,), diagnostic=None):
            nonlocal checks
            result = subprocess.run([str(binaries[name]), *arguments], cwd=root,
                                    text=True, stdout=subprocess.PIPE,
                                    stderr=subprocess.STDOUT, timeout=30)
            assert result.returncode in expected, result.stdout[-5000:]
            if diagnostic is not None:
                assert diagnostic in result.stdout, result.stdout[-5000:]
            checks += 1
            return result

        def runtime_snapshot(name):
            contents = (root / name).read_text()
            return dict(line.split(" = ", 1) for line in
                        contents.split("[runtime]\n", 1)[1].splitlines() if " = " in line)

        for name in binaries:
            run(name, ["--help"], diagnostic="--config")

        for index, invalid in enumerate(("dt_s = nan", "dt_s = 0", "dt_s = 1e999",
                                         "seed = -1", "viewer_enabled = yes",
                                         "dt_s = 0.01\ndt_s = 0.02", "unknown = 1",
                                         "dt_s = 0.1\ntelemetry_rate_hz = 20")):
            path = root / f"invalid-{index}.conf"
            path.write_text(invalid + "\n")
            run("sil_runner", ["--scenario", "NOMINAL-001", "--config", str(path)], (2,))

        run("sil_runner", ["--scenario", "NOMINAL-001", "--config", "missing.conf"],
            (2,), "cannot open")
        run("hil_runner", ["--interface", "loopback", "--config", "missing.conf"],
            (2,), "cannot open")
        run("sil_monte_carlo", ["--config", "missing.conf"], (2,), "cannot open")

        for invalid in ("aircraft.mass_kg = 0", "aircraft.minimum_rpm = 13000",
                        "dispersion.mass_variation_std = nan", "aircraft.mass_kgg = 1.2"):
            (root / "physics-test.conf").write_text(invalid + "\n")
            run("sil_runner", ["--scenario", "NOMINAL-001", "--simulation-config", "physics-test.conf"], (2,))

        for invalid in ("transport_timeout_us = 0", "telemetry_rate_hz = nan",
                        "deadline_policy = Invalid", "dt_s = 0.001"):
            (root / "hil-test.conf").write_text(invalid + "\n")
            run("hil_runner", ["--interface", "loopback", "--config", "hil-test.conf"], (2,))

        scenarios = root / "config/scenarios"
        nominal = scenarios / "NOMINAL-001.conf"
        saved_nominal = nominal.read_text()
        nominal.write_text("duration_s = 0.01\n")
        (root / "sil-test.conf").write_text("viewer_enabled = false\ndt_s = 0.02\n")
        run("sil_runner", ["--scenario", "NOMINAL-001", "--config", "sil-test.conf"], (2,))
        (root / "sil-test.conf").write_text("viewer_enabled = false\n")
        nominal.write_text("report_period_s = 0.001\n")
        run("sil_runner", ["--scenario", "NOMINAL-001", "--config", "sil-test.conf"], (2,))
        nominal.write_text("wind_x_mps = 1\nwind_start_s = 25\nwind_end_s = 20\n")
        run("sil_runner", ["--scenario", "NOMINAL-001", "--config", "sil-test.conf"], (2,))
        nominal.write_text(saved_nominal)

        wind = scenarios / "WIND-002.conf"
        wind.rename(wind.with_suffix(".saved"))
        run("sil_runner", ["--scenario", "NOMINAL-001", "--config", "sil-test.conf"], (2,), "cannot open")
        wind.with_suffix(".saved").rename(wind)
        (root / "not-directory").write_text("snapshot failure test\n")
        run("sil_runner", ["--scenario", "NOMINAL-001", "--config", "sil-test.conf",
                           "--config-output", "not-directory/snapshot.txt"], (2,), "snapshot directory")

        run("sil_runner", ["--scenario", "NOMINAL-001", "--config", "sil-test.conf",
                           "--telemetry-period", "0.2", "--config-output", "sil-snapshot.txt"])
        assert float(runtime_snapshot("sil-snapshot.txt")["effective.report_period_s"]) == 0.2
        checks += 1
        (root / "physics-test.conf").write_text("aircraft.mass_kg = 2.4\n")
        run("sil_runner", ["--scenario", "NOMINAL-001", "--config", "sil-test.conf",
                           "--simulation-config", "physics-test.conf", "--config-output", "physics-snapshot.txt"], (0, 1))
        derived = float(runtime_snapshot("physics-snapshot.txt")["derived.controller.hover_rpm"])
        assert math.isclose(derived, math.sqrt(2) * 815.527280820643, rel_tol=1e-12)
        checks += 1

        windy = run("sil_runner", ["--scenario", "WIND-001", "--config", "sil-test.conf"])
        wind_profile = scenarios / "WIND-001.conf"
        saved_wind = wind_profile.read_text()
        wind_profile.write_text(saved_wind.replace("wind_y_mps = 4", "wind_y_mps = 0"))
        calm = run("sil_runner", ["--scenario", "WIND-001", "--config", "sil-test.conf"])
        numeric_rows = lambda output: [line for line in output.splitlines()
                                       if line.startswith("|") and line.split("|")[1].strip()
                                       and line.split("|")[1].strip()[0].isdigit()]
        assert numeric_rows(windy.stdout) and numeric_rows(windy.stdout) != numeric_rows(calm.stdout)
        checks += 1
        wind_profile.write_text(saved_wind)

        run("hil_runner", ["--interface", "loopback", "--duration", "0.02", "--seed", "123",
                           "--telemetry-period", "0.02", "--config-output", "hil-snapshot.txt"], (0, 1))
        snapshot = runtime_snapshot("hil-snapshot.txt")
        assert snapshot["interface"] == "loopback" and snapshot["seed"] == "123"
        assert float(snapshot["duration_s"]) == 0.02 and float(snapshot["report_period_s"]) == 0.02
        checks += 1

        (root / "campaign.conf").write_text("runs = 3\nseed = 99\nscenario = NOMINAL-001\n")
        for report in ("first.json", "second.json"):
            run("sil_monte_carlo", ["--config", "campaign.conf", "--sil-config", "sil-test.conf",
                                    "--runs", "2", "--seed", "123", "--scenario", "NOMINAL-002",
                                    "--output-json", report, "--config-output", "campaign-snapshot.txt"])
        snapshot = runtime_snapshot("campaign-snapshot.txt")
        assert snapshot["runs"] == "2" and snapshot["seed"] == "123"
        assert snapshot["scenario"] == "NOMINAL-002"
        assert json.loads((root / "first.json").read_text()) == json.loads((root / "second.json").read_text())
        checks += 1
        (root / "physics-test.conf").write_text("dispersion.mass_variation_std = 0\n")
        run("sil_monte_carlo", ["--runs", "2", "--scenario", "NOMINAL-002",
                               "--simulation-config", "physics-test.conf", "--output-json", "zero-mass.json"])
        assert all(record["inputs"]["mass_variation"] == 0 for record in
                   json.loads((root / "zero-mass.json").read_text()))
        checks += 1
        run("sil_monte_carlo", ["--runs", "1", "--scenario", "WIND-002",
                               "--output-json", "wind.json"])
        assert json.loads((root / "wind.json").read_text())[0]["status"] == "PASS"
        checks += 1
        run("sil_monte_carlo", ["--runs", "4294967296"], (2,), "invalid value")

    print(f"PASS: {checks} host configuration CLI checks")


if __name__ == "__main__":
    main()
