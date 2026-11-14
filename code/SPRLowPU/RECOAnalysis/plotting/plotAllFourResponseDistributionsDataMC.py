#!/usr/bin/env python3

import subprocess
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent

CASES = [
    (
        "prePFHadronCalibrationLinkedECALPlusHCALResponseVsPVarBins",
        "Before PF hadron calibration",
        "data_mc_before_pf_hadron_calibration_response_distributions",
    ),
    (
        "pfHadronCalibratedLinkedECALPlusHCALResponseVsPVarBins",
        "After PF hadron calibration",
        "data_mc_after_pf_hadron_calibration_response_distributions",
    ),
    (
        "hcalLinkedPFCandidateResponseVsPVarBins",
        "PF candidate ECAL+HCAL",
        "data_mc_pf_candidate_response_distributions",
    ),
    (
        "hcalLinkedRawPFCandidateResponseVsPVarBins",
        "Raw PF candidate ECAL+HCAL",
        "data_mc_raw_pf_candidate_response_distributions",
    ),
]


def main():
    import argparse

    parser = argparse.ArgumentParser(
        description="Make Data/MC response-distribution plots for all four response definitions."
    )
    parser.add_argument("--data-file", required=True)
    parser.add_argument("--mc-file", required=True)
    parser.add_argument("--output-dir", required=True)
    args = parser.parse_args()

    plotter = SCRIPT_DIR / "plotResponseDistributionsByMomentumDataMC.py"

    for histogram, label, output_name in CASES:
        print(f"\n=== {label} ===")

        subprocess.run(
            [
                str(plotter),
                "--data-file", args.data_file,
                "--mc-file", args.mc_file,
                "--histogram", histogram,
                "--calibration-label", label,
                "--output-dir", args.output_dir,
                "--output-name", output_name,
                "--momentum-max", "15",
            ],
            check=True,
        )


if __name__ == "__main__":
    main()
