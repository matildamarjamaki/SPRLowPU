#!/usr/bin/env python3

import argparse
import math
import sys
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
CMSSTYLE_SRC = SCRIPT_DIR / "cmsstyle" / "src"
sys.path.insert(0, str(CMSSTYLE_SRC))

import ROOT  # noqa: E402
import cmsstyle as CMS  # noqa: E402


DEFAULT_INPUT_FILE = (
    SCRIPT_DIR.parents[2]
    / "output_merged"
    / "sprLowPUData_MC26Winter_Pi_E-0p2to10_E-0p2to200.root"
)
DEFAULT_HISTOGRAM = "prePFHadronCalibrationLinkedECALPlusHCALResponseVsPVarBins"
DEFAULT_OUTPUT_DIR = SCRIPT_DIR / "plots"
DEFAULT_OUTPUT_NAME = "mc_pre_pf_hadron_calibration_response_distributions"

ANALYZER_DIRECTORY = "sprLowPUDataAnalyzer"
N_COLUMNS = 8
CANVAS_WIDTH = 2800
CANVAS_HEIGHT = 1440


def parse_arguments():
    parser = argparse.ArgumentParser(
        description="Project and unit-normalize the response distribution in each track-momentum bin."
    )
    parser.add_argument("--input-file", type=Path, default=DEFAULT_INPUT_FILE, help="Input ROOT file")
    parser.add_argument(
        "--histogram",
        default=DEFAULT_HISTOGRAM,
        help="TH2D name, with or without the analyzer-directory prefix",
    )
    parser.add_argument("--sample-label", default="PionGunNoPU", help="Sample label in the common header")
    parser.add_argument(
        "--calibration-label", default="Pre-PF hadron calibration", help="Calibration label in the common header"
    )
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUTPUT_DIR, help="Plot output directory")
    parser.add_argument("--output-name", default=DEFAULT_OUTPUT_NAME, help="Output filename without an extension")
    parser.add_argument("--response-max", type=float, default=2.0, help="Upper edge of the displayed response range")
    parser.add_argument(
        "--rebin-factor",
        type=int,
        default=2,
        help="Number of adjacent response bins to merge before normalization",
    )
    return parser.parse_args()


def open_root_file(path):
    root_file = ROOT.TFile.Open(str(path), "READ")
    if not root_file or root_file.IsZombie():
        raise RuntimeError(f"Could not open ROOT file: {path}")
    return root_file


def load_histogram(path, histogram_name):
    object_path = histogram_name if "/" in histogram_name else f"{ANALYZER_DIRECTORY}/{histogram_name}"
    root_file = open_root_file(path)
    try:
        histogram = root_file.Get(object_path)
        if not histogram:
            raise RuntimeError(f"Could not find '{object_path}' in {path}")
        if not histogram.InheritsFrom("TH2"):
            raise RuntimeError(f"Object '{object_path}' in {path} is not a TH2")

        histogram_clone = histogram.Clone("responseVsMomentumForProjection")
        histogram_clone.SetDirectory(0)
    finally:
        root_file.Close()

    return histogram_clone


def make_unit_normalized_projections(
    histogram, rebin_factor, projection_prefix="responseProjectionMomentumBin"
):
    if rebin_factor < 1:
        raise RuntimeError(f"rebin-factor must be at least 1, received {rebin_factor}")

    projections = []
    for momentum_bin in range(1, histogram.GetNbinsX() + 1):
        projection = histogram.ProjectionY(
            f"{projection_prefix}{momentum_bin}", momentum_bin, momentum_bin, "e"
        )
        projection.SetDirectory(0)
        projection.SetStats(False)
        if rebin_factor > 1:
            projection.Rebin(rebin_factor)

        integral = projection.Integral(1, projection.GetNbinsX())
        if integral <= 0.0:
            low_edge = histogram.GetXaxis().GetBinLowEdge(momentum_bin)
            high_edge = histogram.GetXaxis().GetBinUpEdge(momentum_bin)
            raise RuntimeError(f"Momentum bin [{low_edge}, {high_edge}) GeV has no in-range response entries")

        projection.Scale(1.0 / integral)
        projections.append(projection)

    return projections


def maximum_bin_height(projections, response_max):
    maximum = 0.0
    for projection in projections:
        last_visible_bin = projection.FindBin(response_max - 1.0e-9)
        for response_bin in range(1, last_visible_bin + 1):
            height = projection.GetBinContent(response_bin) + projection.GetBinError(response_bin)
            maximum = max(maximum, height)
    if maximum <= 0.0:
        raise RuntimeError("The displayed response range contains no entries")

    axis_step = 0.02
    return axis_step * math.ceil(1.15 * maximum / axis_step)


def format_edge(edge):
    return f"{edge:g}"


def momentum_bin_label(histogram, momentum_bin):
    low_edge = histogram.GetXaxis().GetBinLowEdge(momentum_bin)
    high_edge = histogram.GetXaxis().GetBinUpEdge(momentum_bin)
    return f"{format_edge(low_edge)} #leq track p < {format_edge(high_edge)} GeV"


def make_plot(
    histogram,
    projections,
    sample_label,
    calibration_label,
    output_dir,
    output_name,
    response_max,
    comparison_projections=None,
    comparison_labels=None,
    annotation_lines=None,
    y_axis_max=None,
):
    ROOT.gROOT.SetBatch(ROOT.kTRUE)
    CMS.setCMSStyle()
    ROOT.gStyle.SetOptStat(0)
    ROOT.gStyle.SetPadTickX(1)
    ROOT.gStyle.SetPadTickY(1)

    number_of_panels = len(projections)
    number_of_rows = math.ceil(number_of_panels / N_COLUMNS)
    number_of_pads = N_COLUMNS * number_of_rows
    response_min = histogram.GetYaxis().GetXmin()
    stored_response_max = histogram.GetYaxis().GetXmax()
    if response_max <= response_min or response_max > stored_response_max:
        raise RuntimeError(
            f"response-max must be in ({response_min}, {stored_response_max}], received {response_max}"
        )
    if comparison_projections is not None and len(comparison_projections) != number_of_panels:
        raise RuntimeError("The primary and comparison samples have different numbers of momentum bins")
    all_projections = projections + (comparison_projections or [])
    if y_axis_max is not None and y_axis_max <= 0.0:
        raise RuntimeError(f"y-axis maximum must be positive, received {y_axis_max}")
    normalized_max = y_axis_max or maximum_bin_height(all_projections, response_max)
    comparison_color = ROOT.TColor.GetColor("#3B75AF")
    x_axis_divisions = max(1, round(2.0 * (response_max - response_min)))

    canvas = ROOT.TCanvas(
        "responseDistributionsByMomentum", "responseDistributionsByMomentum", CANVAS_WIDTH, CANVAS_HEIGHT
    )
    canvas.SetCanvasSize(CANVAS_WIDTH, CANVAS_HEIGHT)
    canvas.SetMargin(0.0, 0.0, 0.0, 0.0)

    outer_left = 0.045
    outer_bottom = 0.055
    frame_grid_left = 0.075
    frame_grid_right = 0.995
    frame_grid_bottom = 0.100
    frame_grid_top = 0.995
    frame_width = (frame_grid_right - frame_grid_left) / N_COLUMNS
    frame_height = (frame_grid_top - frame_grid_bottom) / number_of_rows

    pads = []
    frames = []
    panel_labels = []

    for pad_index in range(number_of_pads):
        row = pad_index // N_COLUMNS
        column = pad_index % N_COLUMNS
        frame_x_low = frame_grid_left + column * frame_width
        frame_x_high = frame_x_low + frame_width
        frame_y_high = frame_grid_top - row * frame_height
        frame_y_low = frame_y_high - frame_height

        x_low = outer_left if column == 0 else frame_x_low
        x_high = frame_x_high
        y_low = outer_bottom if row == number_of_rows - 1 else frame_y_low
        y_high = frame_y_high

        pad_width = x_high - x_low
        pad_height = y_high - y_low
        left_margin = (frame_x_low - x_low) / pad_width
        bottom_margin = (frame_y_low - y_low) / pad_height

        canvas.cd()
        pad = ROOT.TPad(f"responsePad{pad_index}", "", x_low, y_low, x_high, y_high)
        pad.SetFillColor(ROOT.kWhite)
        pad.SetBorderMode(0)
        pad.SetFrameBorderMode(0)
        pad.SetLeftMargin(left_margin)
        pad.SetRightMargin(0.0)
        pad.SetTopMargin(0.0)
        pad.SetBottomMargin(bottom_margin)
        pad.SetTicks(1, 1)
        pad.Draw()
        pad.cd()

        frame = pad.DrawFrame(response_min, 0.0, response_max, normalized_max)
        frame.SetLineColor(ROOT.kBlack)
        frame.SetLineWidth(1)
        frame.GetXaxis().SetNdivisions(500 + x_axis_divisions, False)
        frame.GetXaxis().SetDecimals(True)
        frame.GetYaxis().SetNdivisions(504, False)
        frame.GetXaxis().SetTickLength(0.025)
        frame.GetYaxis().SetTickLength(0.025)
        frame.GetXaxis().SetLabelOffset(0.010)
        frame.GetYaxis().SetLabelOffset(0.010)
        # Font precision 3 interprets label sizes in pixels, keeping them
        # identical even for pads that include the outer label margins.
        frame.GetXaxis().SetLabelFont(43)
        frame.GetYaxis().SetLabelFont(43)
        frame.GetXaxis().SetLabelSize(16 if row == number_of_rows - 1 else 0)
        frame.GetYaxis().SetLabelSize(16 if column == 0 else 0)

        if pad_index < number_of_panels:
            projection = projections[pad_index]
            if comparison_projections is not None:
                CMS.cmsDraw(
                    comparison_projections[pad_index],
                    "HIST",
                    mcolor=comparison_color,
                    lstyle=ROOT.kSolid,
                    lwidth=1,
                    lcolor=comparison_color,
                    fstyle=0,
                )
                CMS.cmsDraw(
                    projection,
                    "E1P",
                    marker=ROOT.kFullCircle,
                    msize=0.8,
                    mcolor=ROOT.kBlack,
                    lstyle=ROOT.kSolid,
                    lwidth=1,
                    lcolor=ROOT.kBlack,
                    fstyle=0,
                )
            else:
                CMS.cmsDraw(
                    projection,
                    "HIST",
                    mcolor=ROOT.kBlack,
                    lstyle=ROOT.kSolid,
                    lwidth=2,
                    lcolor=ROOT.kBlack,
                    fstyle=0,
                )

            panel_label = ROOT.TLatex()
            panel_label.SetNDC(True)
            panel_label.SetTextFont(43)
            panel_label.SetTextSize(16)
            panel_label.SetTextAlign(23)
            frame_center = 0.5 * (pad.GetLeftMargin() + 1.0 - pad.GetRightMargin())
            panel_label.DrawLatex(frame_center, 0.94, momentum_bin_label(histogram, pad_index + 1))
            panel_labels.append(panel_label)

        pads.append(pad)
        frames.append(frame)

    canvas.cd()
    overlay_pad = ROOT.TPad("responseLabelsOverlay", "", 0.0, 0.0, 1.0, 1.0)
    overlay_pad.SetFillStyle(4000)
    overlay_pad.SetFrameFillStyle(0)
    overlay_pad.SetBorderMode(0)
    overlay_pad.SetMargin(0.0, 0.0, 0.0, 0.0)
    overlay_pad.Draw()
    overlay_pad.cd()

    shared_y_title = ROOT.TLatex()
    shared_y_title.SetNDC(True)
    shared_y_title.SetTextFont(42)
    shared_y_title.SetTextSize(0.020)
    shared_y_title.SetTextAngle(90)
    shared_y_title.SetTextAlign(22)
    shared_y_title.DrawLatex(
        0.013, 0.5 * (frame_grid_bottom + frame_grid_top), "Tracks (Normalized to Unity)"
    )

    shared_x_title = ROOT.TLatex()
    shared_x_title.SetNDC(True)
    shared_x_title.SetTextFont(42)
    shared_x_title.SetTextSize(0.024)
    shared_x_title.SetTextAlign(22)
    shared_x_title.DrawLatex(
        0.5 * (frame_grid_left + frame_grid_right), 0.016, "(E_{ECAL} + E_{HCAL}) / p"
    )

    panel_annotations = []
    displayed_annotation_lines = annotation_lines or (sample_label, calibration_label, "2026 (13.6 TeV)")
    for line_index, annotation_text in enumerate(displayed_annotation_lines):
        annotation = ROOT.TLatex()
        annotation.SetNDC(True)
        annotation.SetTextFont(42)
        annotation.SetTextSize(0.010)
        annotation.SetTextAlign(33)
        annotation.DrawLatex(
            frame_grid_right - 0.010,
            frame_grid_top - 0.055 - 0.025 * line_index,
            annotation_text,
        )
        panel_annotations.append(annotation)

    comparison_legend = None
    if comparison_projections is not None:
        if comparison_labels is None or len(comparison_labels) != 2:
            raise RuntimeError("Two comparison labels are required for a two-sample plot")
        comparison_legend = CMS.cmsLeg(0.880, 0.660, 0.992, 0.725, textSize=0.010)
        comparison_legend.AddEntry(projections[0], comparison_labels[0], "ep")
        comparison_legend.AddEntry(comparison_projections[0], comparison_labels[1], "l")

    canvas.cd()
    canvas.Modified()
    canvas.Update()

    output_dir.mkdir(parents=True, exist_ok=True)
    output_stem = output_dir / output_name
    pdf_path = output_stem.with_suffix(".pdf")
    png_path = output_stem.with_suffix(".png")
    CMS.SaveCanvas(canvas, str(pdf_path), close=False)
    CMS.SaveCanvas(canvas, str(png_path))

    return pdf_path, png_path


def main():
    args = parse_arguments()

    histogram = load_histogram(args.input_file.resolve(), args.histogram)
    projections = make_unit_normalized_projections(histogram, args.rebin_factor)
    output_files = make_plot(
        histogram,
        projections,
        args.sample_label,
        args.calibration_label,
        args.output_dir.resolve(),
        args.output_name,
        args.response_max,
    )

    for output_file in output_files:
        print(f"Wrote {output_file}")


if __name__ == "__main__":
    try:
        main()
    except RuntimeError as error:
        print(f"ERROR: {error}", file=sys.stderr)
        sys.exit(1)
