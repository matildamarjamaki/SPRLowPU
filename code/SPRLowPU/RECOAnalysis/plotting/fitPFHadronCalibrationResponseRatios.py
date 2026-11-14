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
    SCRIPT_DIR / "plots" / "pf_hadron_calibration_response_vs_p_data_mc_ratio.root"
)
DEFAULT_OUTPUT_DIR = SCRIPT_DIR / "plots"
DEFAULT_OUTPUT_NAME = "pf_hadron_calibration_data_mc_ratio_fits"
DEFAULT_FIT_FORMULA = "[0] + [1]*log(x) + [2]*pow(log(x),2)"

RATIO_PATHS = {
    "before": "ratios/data_over_mc_before_pf_hadron_calibration",
    "after": "ratios/data_over_mc_after_pf_hadron_calibration",
}


def parse_arguments():
    parser = argparse.ArgumentParser(
        description="Fit the Data/MC response ratios before and after PF hadron calibration."
    )
    parser.add_argument("--input-file", type=Path, default=DEFAULT_INPUT_FILE, help="Input ROOT file")
    parser.add_argument("--fit-min", type=float, default=3.0, help="Lower edge of the fit range in GeV")
    parser.add_argument("--fit-max", type=float, default=80.0, help="Upper edge of the fit range in GeV")
    parser.add_argument(
        "--fit-formula",
        default=DEFAULT_FIT_FORMULA,
        help="Formula passed to ROOT.TF1",
    )
    parser.add_argument("--ratio-min", type=float, default=0.8, help="Lower edge of the ratio axis")
    parser.add_argument("--ratio-max", type=float, default=1.3, help="Upper edge of the ratio axis")
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUTPUT_DIR, help="Plot output directory")
    parser.add_argument("--output-name", default=DEFAULT_OUTPUT_NAME, help="Output filename without an extension")
    parser.add_argument(
        "--root-output",
        type=Path,
        default=None,
        help="Fit ROOT output file (default: <output-dir>/<output-name>.root)",
    )
    return parser.parse_args()


def open_root_file(path):
    root_file = ROOT.TFile.Open(str(path), "READ")
    if not root_file or root_file.IsZombie():
        raise RuntimeError(f"Could not open ROOT file: {path}")
    return root_file


def load_ratios(path):
    input_file = open_root_file(path)
    ratios = {}
    try:
        for calibration, object_path in RATIO_PATHS.items():
            ratio = input_file.Get(object_path)
            if not ratio:
                raise RuntimeError(f"Could not find '{object_path}' in {path}")
            if not ratio.InheritsFrom("TH1D"):
                raise RuntimeError(f"Object '{object_path}' in {path} is not a TH1D")

            ratio_clone = ratio.Clone(f"fit_input_{calibration}_ratio")
            ratio_clone.SetDirectory(0)
            ratio_clone.SetStats(False)
            ratios[calibration] = ratio_clone
    finally:
        input_file.Close()

    return ratios


def validate_common_binning(ratios):
    reference = ratios["before"]
    comparison = ratios["after"]
    if reference.GetNbinsX() != comparison.GetNbinsX():
        raise RuntimeError("The before- and after-calibration ratios have different bin counts")

    for bin_index in range(1, reference.GetNbinsX() + 2):
        reference_edge = reference.GetXaxis().GetBinLowEdge(bin_index)
        comparison_edge = comparison.GetXaxis().GetBinLowEdge(bin_index)
        if abs(reference_edge - comparison_edge) > 1.0e-9:
            raise RuntimeError("The before- and after-calibration ratios have different bin edges")


def count_usable_bins(histogram, fit_min, fit_max):
    usable_bins = 0
    for bin_index in range(1, histogram.GetNbinsX() + 1):
        bin_center = histogram.GetXaxis().GetBinCenter(bin_index)
        bin_content = histogram.GetBinContent(bin_index)
        bin_error = histogram.GetBinError(bin_index)
        if (
            fit_min <= bin_center <= fit_max
            and math.isfinite(bin_content)
            and math.isfinite(bin_error)
            and bin_error > 0.0
        ):
            usable_bins += 1
    return usable_bins


def fit_ratios(ratios, fit_formula, fit_min, fit_max):
    if fit_min >= fit_max:
        raise RuntimeError("fit-min must be smaller than fit-max")

    axis_min = ratios["before"].GetXaxis().GetXmin()
    axis_max = ratios["before"].GetXaxis().GetXmax()
    if fit_min < axis_min or fit_max > axis_max:
        raise RuntimeError(
            f"Fit range [{fit_min}, {fit_max}] lies outside histogram range [{axis_min}, {axis_max}]"
        )

    fit_functions = {}
    fit_results = {}
    for calibration in ("before", "after"):
        fit_function = ROOT.TF1(
            f"{calibration}_pf_hadron_calibration_fit",
            fit_formula,
            fit_min,
            fit_max,
        )
        if not fit_function.IsValid():
            raise RuntimeError(f"ROOT could not construct a valid TF1 from '{fit_formula}'")

        for parameter_index in range(fit_function.GetNpar()):
            fit_function.SetParameter(parameter_index, 0.0)
        if fit_function.GetNpar() > 0:
            fit_function.SetParameter(0, 1.0)
        if fit_formula == DEFAULT_FIT_FORMULA:
            fit_function.SetParNames("Constant", "LogSlope", "LogCurvature")

        usable_bins = count_usable_bins(ratios[calibration], fit_min, fit_max)
        if usable_bins <= fit_function.GetNpar():
            raise RuntimeError(
                f"The {calibration}-calibration ratio has only {usable_bins} usable bins "
                f"for a {fit_function.GetNpar()}-parameter fit"
            )

        fit_result = ratios[calibration].Fit(fit_function, "SQNR")
        fit_status = int(fit_result)
        if fit_status != 0:
            raise RuntimeError(
                f"The {calibration}-calibration fit failed with ROOT status {fit_status}"
            )

        fit_functions[calibration] = fit_function
        fit_results[calibration] = fit_result

    return fit_functions, fit_results


def print_fit_results(fit_functions, fit_results):
    for calibration in ("before", "after"):
        fit_function = fit_functions[calibration]
        fit_result = fit_results[calibration]
        chi_square = fit_result.Chi2()
        ndf = fit_result.Ndf()
        probability = ROOT.TMath.Prob(chi_square, ndf) if ndf > 0 else float("nan")

        print(f"{calibration.capitalize()} PF hadron calibration fit:")
        print(f"  status = {int(fit_result)}")
        for parameter_index in range(fit_function.GetNpar()):
            print(
                f"  {fit_function.GetParName(parameter_index)} = "
                f"{fit_result.Parameter(parameter_index):.8g} +/- "
                f"{fit_result.ParError(parameter_index):.8g}"
            )
        print(f"  chi2 / ndf = {chi_square:.6g} / {ndf}")
        print(f"  probability = {probability:.6g}")


def format_fit_annotation(fit_function, fit_result):
    lines = []
    for parameter_index in range(fit_function.GetNpar()):
        lines.append(
            f"p_{{{parameter_index}}} = {fit_result.Parameter(parameter_index):.4g} "
            f"#pm {fit_result.ParError(parameter_index):.2g}"
        )
    lines.append(f"#chi^{{2}}/ndf = {fit_result.Chi2():.1f}/{fit_result.Ndf()}")
    return lines


def make_plot(ratios, fit_functions, fit_results, fit_min, fit_max, ratio_min, ratio_max, output_dir, output_name):
    if ratio_min >= ratio_max:
        raise RuntimeError("ratio-min must be smaller than ratio-max")

    ROOT.gROOT.SetBatch(ROOT.kTRUE)
    CMS.SetCmsText("")
    CMS.SetExtraText("LowPU(1-2)")
    CMS.SetLumi(None, run="2026C")
    CMS.SetEnergy(13.6)

    plot_min = ratios["before"].GetXaxis().GetXmin()
    plot_max = ratios["before"].GetXaxis().GetXmax()

    canvas = CMS.cmsCanvas(
        "pfHadronCalibrationDataMCRatioFits",
        plot_min,
        plot_max,
        ratio_min,
        ratio_max,
        "Track momentum p [GeV]",
        "Data / MC",
        square=True,
        iPos=0,
        extraSpace=0.04,
    )
    canvas.SetCanvasSize(900, 900)
    canvas.SetLogx(True)
    axis_histogram = canvas.GetPrimitive("hframe")
    if axis_histogram:
        axis_histogram.GetXaxis().SetMoreLogLabels(True)
        axis_histogram.GetXaxis().SetNoExponent(True)

    before_color = ROOT.TColor.GetColor("#E07B39")
    after_color = ROOT.TColor.GetColor("#3B75AF")
    colors = {"before": before_color, "after": after_color}

    unity_line = ROOT.TLine(plot_min, 1.0, plot_max, 1.0)
    CMS.cmsDrawLine(unity_line, lcolor=ROOT.kGray + 2, lstyle=ROOT.kDotted, lwidth=2)

    for calibration in ("before", "after"):
        CMS.cmsDraw(
            ratios[calibration],
            "E1P",
            marker=ROOT.kFullCircle,
            msize=1.0,
            mcolor=colors[calibration],
            lstyle=ROOT.kSolid,
            lwidth=1,
            lcolor=colors[calibration],
            fstyle=0,
        )
        CMS.cmsDraw(
            fit_functions[calibration],
            "L",
            mcolor=colors[calibration],
            lstyle=ROOT.kSolid,
            lwidth=3,
            lcolor=colors[calibration],
            fstyle=0,
        )

    legend = CMS.cmsLeg(0.19, 0.72, 0.52, 0.88, textSize=0.030)
    legend.AddEntry(ratios["before"], "Before PFHC", "lep")
    legend.AddEntry(ratios["after"], "After PFHC", "lep")

    annotation_objects = []
    annotation_columns = {"before": 0.19, "after": 0.59}
    annotation_titles = {"before": "Before PFHC fit", "after": "After PFHC fit"}
    for calibration in ("before", "after"):
        annotation = ROOT.TLatex()
        annotation.SetNDC(True)
        annotation.SetTextFont(42)
        annotation.SetTextSize(0.022)
        annotation.SetTextColor(colors[calibration])
        annotation.DrawLatex(annotation_columns[calibration], 0.34, annotation_titles[calibration])
        for line_index, text in enumerate(
            format_fit_annotation(fit_functions[calibration], fit_results[calibration])
        ):
            annotation.DrawLatex(
                annotation_columns[calibration], 0.305 - 0.030 * line_index, text
            )
        annotation_objects.append(annotation)

    canvas.RedrawAxis()
    output_dir.mkdir(parents=True, exist_ok=True)
    output_stem = output_dir / output_name
    pdf_path = output_stem.with_suffix(".pdf")
    png_path = output_stem.with_suffix(".png")
    CMS.SaveCanvas(canvas, str(pdf_path), close=False)
    CMS.SaveCanvas(canvas, str(png_path))
    return pdf_path, png_path


def write_fit_output(ratios, fit_functions, fit_results, output_path):
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_file = ROOT.TFile.Open(str(output_path), "RECREATE")
    if not output_file or output_file.IsZombie():
        raise RuntimeError(f"Could not create ROOT output file: {output_path}")

    try:
        ratios_directory = output_file.mkdir("ratios")
        fits_directory = output_file.mkdir("fits")
        results_directory = output_file.mkdir("fit_results")
        if not ratios_directory or not fits_directory or not results_directory:
            raise RuntimeError(f"Could not create directories in ROOT output file: {output_path}")

        for calibration in ("before", "after"):
            ratio_name = f"data_over_mc_{calibration}_pf_hadron_calibration"
            fit_name = f"{calibration}_pf_hadron_calibration_fit"
            result_name = f"{calibration}_pf_hadron_calibration_fit_result"

            ratios_directory.cd()
            if ratios[calibration].Write(ratio_name, ROOT.TObject.kOverwrite) <= 0:
                raise RuntimeError(f"Could not write ratios/{ratio_name} to {output_path}")

            fits_directory.cd()
            if fit_functions[calibration].Write(fit_name, ROOT.TObject.kOverwrite) <= 0:
                raise RuntimeError(f"Could not write fits/{fit_name} to {output_path}")

            results_directory.cd()
            if fit_results[calibration].Get().Write(result_name, ROOT.TObject.kOverwrite) <= 0:
                raise RuntimeError(f"Could not write fit_results/{result_name} to {output_path}")

        output_file.Write()
    finally:
        output_file.Close()

    expected_objects = {
        "ratios/data_over_mc_before_pf_hadron_calibration": "TH1D",
        "ratios/data_over_mc_after_pf_hadron_calibration": "TH1D",
        "fits/before_pf_hadron_calibration_fit": "TF1",
        "fits/after_pf_hadron_calibration_fit": "TF1",
        "fit_results/before_pf_hadron_calibration_fit_result": "TFitResult",
        "fit_results/after_pf_hadron_calibration_fit_result": "TFitResult",
    }
    validation_file = open_root_file(output_path)
    try:
        for object_path, expected_type in expected_objects.items():
            saved_object = validation_file.Get(object_path)
            if not saved_object:
                raise RuntimeError(f"Missing '{object_path}' in ROOT output file: {output_path}")
            if not saved_object.InheritsFrom(expected_type):
                raise RuntimeError(f"'{object_path}' in {output_path} is not a {expected_type}")
    finally:
        validation_file.Close()

    return output_path


def main():
    args = parse_arguments()
    output_dir = args.output_dir.resolve()
    root_output_path = (
        args.root_output.resolve()
        if args.root_output is not None
        else output_dir / f"{args.output_name}.root"
    )

    ratios = load_ratios(args.input_file.resolve())
    validate_common_binning(ratios)
    fit_functions, fit_results = fit_ratios(
        ratios, args.fit_formula, args.fit_min, args.fit_max
    )
    print_fit_results(fit_functions, fit_results)

    plot_files = make_plot(
        ratios,
        fit_functions,
        fit_results,
        args.fit_min,
        args.fit_max,
        args.ratio_min,
        args.ratio_max,
        output_dir,
        args.output_name,
    )
    root_output_file = write_fit_output(
        ratios, fit_functions, fit_results, root_output_path
    )

    for output_file in (*plot_files, root_output_file):
        print(f"Wrote {output_file}")


if __name__ == "__main__":
    try:
        main()
    except RuntimeError as error:
        print(f"ERROR: {error}", file=sys.stderr)
        sys.exit(1)
