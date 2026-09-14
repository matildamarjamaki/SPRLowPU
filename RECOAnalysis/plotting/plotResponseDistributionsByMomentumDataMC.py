#!/usr/bin/env python3

import argparse
import sys
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(SCRIPT_DIR))

import plotResponseDistributionsByMomentum as response_plot  # noqa: E402


DEFAULT_INPUT_DIR = SCRIPT_DIR.parents[2] / "output_merged"
DEFAULT_DATA_FILE = DEFAULT_INPUT_DIR / "sprLowPUData_Data26C_LowPU1or2_EphemeralZeroBias.root"
DEFAULT_MC_FILE = DEFAULT_INPUT_DIR / "sprLowPUData_MC26Winter_Pi_E-0p2to10_E-0p2to200.root"
DEFAULT_HISTOGRAM = "prePFHadronCalibrationLinkedECALPlusHCALResponseVsPVarBins"
DEFAULT_OUTPUT_DIR = SCRIPT_DIR / "plots"
DEFAULT_OUTPUT_NAME = "data_mc_pre_pf_hadron_calibration_response_distributions"


def parse_arguments():
    parser = argparse.ArgumentParser(
        description="Compare unit-normalized Data and MC response distributions in each momentum bin."
    )
    parser.add_argument("--data-file", type=Path, default=DEFAULT_DATA_FILE, help="Merged Data ROOT file")
    parser.add_argument("--mc-file", type=Path, default=DEFAULT_MC_FILE, help="Merged MC ROOT file")
    parser.add_argument(
        "--histogram",
        default=DEFAULT_HISTOGRAM,
        help="TH2D name, with or without the analyzer-directory prefix",
    )
    parser.add_argument("--data-label", default="Data (EphemeralZeroBias)", help="Data legend label")
    parser.add_argument("--mc-label", default="MC (PionGunNoPU)", help="MC legend label")
    parser.add_argument(
        "--calibration-label",
        default="Pre-PF hadron calibration",
        help="Calibration label in the common annotation",
    )
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUTPUT_DIR, help="Plot output directory")
    parser.add_argument("--output-name", default=DEFAULT_OUTPUT_NAME, help="Output filename without an extension")
    parser.add_argument("--response-max", type=float, default=3.0, help="Upper edge of the displayed response range")
    parser.add_argument("--y-max", type=float, default=0.3, help="Upper edge of the normalized-distribution axis")
    parser.add_argument(
        "--rebin-factor",
        type=int,
        default=2,
        help="Number of adjacent response bins to merge before normalization",
    )
    return parser.parse_args()


def validate_common_axis(reference_axis, comparison_axis, axis_name):
    if reference_axis.GetNbins() != comparison_axis.GetNbins():
        raise RuntimeError(f"Data and MC have different numbers of {axis_name} bins")

    for bin_index in range(1, reference_axis.GetNbins() + 2):
        reference_edge = reference_axis.GetBinLowEdge(bin_index)
        comparison_edge = comparison_axis.GetBinLowEdge(bin_index)
        if abs(reference_edge - comparison_edge) > 1.0e-9:
            raise RuntimeError(f"Data and MC do not have identical {axis_name}-bin edges")


def validate_common_binning(data_histogram, mc_histogram):
    validate_common_axis(data_histogram.GetXaxis(), mc_histogram.GetXaxis(), "momentum")
    validate_common_axis(data_histogram.GetYaxis(), mc_histogram.GetYaxis(), "response")


def main():
    args = parse_arguments()

    data_histogram = response_plot.load_histogram(args.data_file.resolve(), args.histogram)
    mc_histogram = response_plot.load_histogram(args.mc_file.resolve(), args.histogram)
    validate_common_binning(data_histogram, mc_histogram)

    data_projections = response_plot.make_unit_normalized_projections(
        data_histogram, args.rebin_factor, "dataResponseProjectionMomentumBin"
    )
    mc_projections = response_plot.make_unit_normalized_projections(
        mc_histogram, args.rebin_factor, "mcResponseProjectionMomentumBin"
    )

    output_files = response_plot.make_plot(
        data_histogram,
        data_projections,
        args.data_label,
        args.calibration_label,
        args.output_dir.resolve(),
        args.output_name,
        args.response_max,
        comparison_projections=mc_projections,
        comparison_labels=(args.data_label, args.mc_label),
        annotation_lines=(args.calibration_label, "LowPU(1-2)", "2026C (13.6 TeV)"),
        y_axis_max=args.y_max,
    )

    for output_file in output_files:
        print(f"Wrote {output_file}")


if __name__ == "__main__":
    try:
        main()
    except RuntimeError as error:
        print(f"ERROR: {error}", file=sys.stderr)
        sys.exit(1)
