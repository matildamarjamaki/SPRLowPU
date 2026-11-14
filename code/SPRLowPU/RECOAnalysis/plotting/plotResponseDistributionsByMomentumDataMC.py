#!/usr/bin/env python3

import argparse
import sys
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(SCRIPT_DIR))

import plotResponseDistributionsByMomentum as response_plot  # noqa: E402


DEFAULT_INPUT_DIR = SCRIPT_DIR.parents[2] / "output_merged"
DEFAULT_DATA_FILE = DEFAULT_INPUT_DIR / "sprLowPUData_Data26C_LowPU1or2_EphemeralZeroBias.root"
DEFAULT_MC_LOW_FILE = DEFAULT_INPUT_DIR / "mc_0p2to10_tmp.root"
DEFAULT_MC_HIGH_FILE = DEFAULT_INPUT_DIR / "mc_0p2to200_tmp.root"

DEFAULT_HISTOGRAM = "prePFHadronCalibrationLinkedECALPlusHCALResponseVsPVarBins"
DEFAULT_OUTPUT_DIR = SCRIPT_DIR / "plots"
DEFAULT_OUTPUT_NAME = "data_mc_pre_pf_hadron_calibration_response_distributions"


def parse_arguments():
    parser = argparse.ArgumentParser(
        description="Compare unit-normalized Data and MC response distributions in each momentum bin."
    )

    parser.add_argument(
        "--data-file",
        type=Path,
        default=DEFAULT_DATA_FILE,
        help="Merged Data ROOT file",
    )

    parser.add_argument(
        "--mc-low-file",
        type=Path,
        default=DEFAULT_MC_LOW_FILE,
        help="MC ROOT file used below the MC switch momentum",
    )

    parser.add_argument(
        "--mc-high-file",
        type=Path,
        default=DEFAULT_MC_HIGH_FILE,
        help="MC ROOT file used at and above the MC switch momentum",
    )

    parser.add_argument(
        "--mc-switch",
        type=float,
        default=10.0,
        help="Momentum [GeV] at which to switch from low-energy to high-energy MC",
    )

    parser.add_argument(
        "--histogram",
        default=DEFAULT_HISTOGRAM,
        help="TH2D name, with or without the analyzer-directory prefix",
    )

    parser.add_argument(
        "--data-label",
        default="Data (EphemeralZeroBias)",
        help="Data legend label",
    )

    parser.add_argument(
        "--mc-label",
        default="MC (PionGunNoPU)",
        help="MC legend label",
    )

    parser.add_argument(
        "--calibration-label",
        default="Pre-PF hadron calibration",
        help="Calibration label in the common annotation",
    )

    parser.add_argument(
        "--output-dir",
        type=Path,
        default=DEFAULT_OUTPUT_DIR,
        help="Plot output directory",
    )

    parser.add_argument(
        "--output-name",
        default=DEFAULT_OUTPUT_NAME,
        help="Output filename without an extension",
    )

    parser.add_argument(
        "--response-max",
        type=float,
        default=3.0,
        help="Upper edge of the displayed response range",
    )

    parser.add_argument(
        "--momentum-max",
        type=float,
        default=15.0,
        help="Upper edge of the displayed track-momentum range",
    )

    parser.add_argument(
        "--y-max",
        type=float,
        default=0.3,
        help="Upper edge of the normalized-distribution axis",
    )

    parser.add_argument(
        "--rebin-factor",
        type=int,
        default=2,
        help="Number of adjacent response bins to merge before normalization",
    )

    return parser.parse_args()


def validate_common_axis(reference_axis, comparison_axis, axis_name):
    if reference_axis.GetNbins() != comparison_axis.GetNbins():
        raise RuntimeError(
            f"Histograms have different numbers of {axis_name} bins"
        )

    for bin_index in range(1, reference_axis.GetNbins() + 2):
        reference_edge = reference_axis.GetBinLowEdge(bin_index)
        comparison_edge = comparison_axis.GetBinLowEdge(bin_index)

        if abs(reference_edge - comparison_edge) > 1.0e-9:
            raise RuntimeError(
                f"Histograms do not have identical {axis_name}-bin edges"
            )


def validate_common_binning(reference_histogram, comparison_histogram):
    validate_common_axis(
        reference_histogram.GetXaxis(),
        comparison_histogram.GetXaxis(),
        "momentum",
    )

    validate_common_axis(
        reference_histogram.GetYaxis(),
        comparison_histogram.GetYaxis(),
        "response",
    )


def make_hybrid_mc_projections(
    low_histogram,
    high_histogram,
    rebin_factor,
    momentum_max,
    switch_momentum,
):
    if rebin_factor < 1:
        raise RuntimeError(
            f"rebin-factor must be at least 1, received {rebin_factor}"
        )

    last_momentum_bin = low_histogram.GetNbinsX()

    if momentum_max is not None:
        last_momentum_bin = low_histogram.GetXaxis().FindBin(
            momentum_max - 1.0e-9
        )

    projections = []

    for momentum_bin in range(1, last_momentum_bin + 1):

        low_edge = low_histogram.GetXaxis().GetBinLowEdge(momentum_bin)
        high_edge = low_histogram.GetXaxis().GetBinUpEdge(momentum_bin)

        if low_edge < switch_momentum:
            source_histogram = low_histogram
            source_label = "0.2-10"
        else:
            source_histogram = high_histogram
            source_label = "0.2-200"

        projection = source_histogram.ProjectionY(
            f"mcHybridResponseProjectionMomentumBin{momentum_bin}",
            momentum_bin,
            momentum_bin,
            "e",
        )

        projection.SetDirectory(0)
        projection.SetStats(False)

        if rebin_factor > 1:
            projection.Rebin(rebin_factor)

        integral = projection.Integral(
            1,
            projection.GetNbinsX(),
        )

        if integral <= 0.0:
            raise RuntimeError(
                f"Selected MC sample {source_label} has no in-range "
                f"response entries in momentum bin "
                f"[{low_edge}, {high_edge}) GeV"
            )

        projection.Scale(1.0 / integral)
        projections.append(projection)

    return projections


def main():
    args = parse_arguments()

    data_histogram = response_plot.load_histogram(
        args.data_file.resolve(),
        args.histogram,
    )

    mc_low_histogram = response_plot.load_histogram(
        args.mc_low_file.resolve(),
        args.histogram,
    )

    mc_high_histogram = response_plot.load_histogram(
        args.mc_high_file.resolve(),
        args.histogram,
    )

    validate_common_binning(data_histogram, mc_low_histogram)
    validate_common_binning(data_histogram, mc_high_histogram)

    data_projections = response_plot.make_unit_normalized_projections(
        data_histogram,
        args.rebin_factor,
        "dataResponseProjectionMomentumBin",
        args.momentum_max,
    )

    mc_projections = make_hybrid_mc_projections(
        mc_low_histogram,
        mc_high_histogram,
        args.rebin_factor,
        args.momentum_max,
        args.mc_switch,
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
        annotation_lines=(
            args.calibration_label,
            "LowPU(1-2)",
            "2026C (13.6 TeV)",
        ),
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
