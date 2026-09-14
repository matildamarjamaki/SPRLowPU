#!/usr/bin/env python3

import argparse
import math
import sys
from array import array
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
CMSSTYLE_SRC = SCRIPT_DIR / "cmsstyle" / "src"
sys.path.insert(0, str(CMSSTYLE_SRC))

import ROOT  # noqa: E402
import cmsstyle as CMS  # noqa: E402


DEFAULT_INPUT_DIR = SCRIPT_DIR.parents[2] / "output_merged"
DEFAULT_DATA_FILE = DEFAULT_INPUT_DIR / "sprLowPUData_Data26C_LowPU1or2_EphemeralZeroBias.root"
DEFAULT_MC_FILE = DEFAULT_INPUT_DIR / "sprLowPUData_MC26Winter_Pi_E-0p2to10_E-0p2to200.root"
DEFAULT_OUTPUT_DIR = SCRIPT_DIR / "plots"
DEFAULT_OUTPUT_NAME = "pf_hadron_calibration_response_vs_p_data_mc_ratio"

ANALYZER_DIRECTORY = "sprLowPUDataAnalyzer"
PRE_CALIBRATION_PROFILE = "meanPrePFHadronCalibrationLinkedECALPlusHCALResponseVsPVarBins"
POST_CALIBRATION_PROFILE = "meanPFHadronCalibratedLinkedECALPlusHCALResponseVsPVarBins"


def parse_arguments():
    parser = argparse.ArgumentParser(
        description="Compare PF-cluster response in Data and MC with a Data/MC ratio pad."
    )
    parser.add_argument("--data-file", type=Path, default=DEFAULT_DATA_FILE, help="Merged Data ROOT file")
    parser.add_argument("--mc-file", type=Path, default=DEFAULT_MC_FILE, help="Merged MC ROOT file")
    parser.add_argument("--ratio-min", type=float, default=0.9, help="Lower edge of the ratio axis")
    parser.add_argument("--ratio-max", type=float, default=1.1, help="Upper edge of the ratio axis")
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUTPUT_DIR, help="Plot output directory")
    parser.add_argument("--output-name", default=DEFAULT_OUTPUT_NAME, help="Output filename without an extension")
    parser.add_argument(
        "--root-output",
        type=Path,
        default=None,
        help="ROOT output file (default: <output-dir>/<output-name>.root)",
    )
    return parser.parse_args()


def open_root_file(path):
    root_file = ROOT.TFile.Open(str(path), "READ")
    if not root_file or root_file.IsZombie():
        raise RuntimeError(f"Could not open ROOT file: {path}")
    return root_file


def load_profile(root_file, profile_name, clone_name):
    object_path = f"{ANALYZER_DIRECTORY}/{profile_name}"
    profile = root_file.Get(object_path)
    if not profile:
        raise RuntimeError(f"Could not find '{object_path}' in {root_file.GetName()}")
    if not profile.InheritsFrom("TProfile"):
        raise RuntimeError(f"Object '{object_path}' in {root_file.GetName()} is not a TProfile")

    profile_clone = profile.Clone(clone_name)
    profile_clone.SetDirectory(0)
    profile_clone.SetStats(False)
    return profile_clone


def load_profiles(path, sample_name):
    root_file = open_root_file(path)
    try:
        profiles = {
            "before": load_profile(
                root_file, PRE_CALIBRATION_PROFILE, f"{sample_name}_response_before_pf_hadron_calibration"
            ),
            "after": load_profile(
                root_file, POST_CALIBRATION_PROFILE, f"{sample_name}_response_after_pf_hadron_calibration"
            ),
        }
    finally:
        root_file.Close()
    return profiles


def validate_common_binning(profiles):
    reference = profiles[0]
    for profile in profiles[1:]:
        if profile.GetNbinsX() != reference.GetNbinsX():
            raise RuntimeError("The Data and MC profiles do not have the same number of momentum bins")

        for bin_index in range(1, reference.GetNbinsX() + 2):
            reference_edge = reference.GetXaxis().GetBinLowEdge(bin_index)
            profile_edge = profile.GetXaxis().GetBinLowEdge(bin_index)
            if abs(profile_edge - reference_edge) > 1.0e-9:
                raise RuntimeError("The Data and MC profiles do not have identical momentum-bin edges")


def make_ratio_histogram(data_profile, mc_profile, name):
    bin_edges = array(
        "d",
        [
            data_profile.GetXaxis().GetBinLowEdge(bin_index)
            for bin_index in range(1, data_profile.GetNbinsX() + 2)
        ],
    )
    ratio = ROOT.TH1D(name, "", data_profile.GetNbinsX(), bin_edges)
    ratio.SetDirectory(0)
    ratio.SetStats(False)

    for bin_index in range(1, ratio.GetNbinsX() + 1):
        data_value = data_profile.GetBinContent(bin_index)
        data_error = data_profile.GetBinError(bin_index)
        mc_value = mc_profile.GetBinContent(bin_index)
        mc_error = mc_profile.GetBinError(bin_index)
        if mc_value == 0.0:
            continue

        ratio_value = data_value / mc_value
        ratio_error = math.hypot(data_error / mc_value, data_value * mc_error / (mc_value * mc_value))
        ratio.SetBinContent(bin_index, ratio_value)
        ratio.SetBinError(bin_index, ratio_error)

    return ratio


def write_root_output(data_profiles, mc_profiles, ratios, output_path):
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_file = ROOT.TFile.Open(str(output_path), "RECREATE")
    if not output_file or output_file.IsZombie():
        raise RuntimeError(f"Could not create ROOT output file: {output_path}")

    profile_objects = {
        "data_before_pf_hadron_calibration": data_profiles["before"],
        "data_after_pf_hadron_calibration": data_profiles["after"],
        "mc_before_pf_hadron_calibration": mc_profiles["before"],
        "mc_after_pf_hadron_calibration": mc_profiles["after"],
    }
    ratio_objects = {
        "data_over_mc_before_pf_hadron_calibration": ratios["before"],
        "data_over_mc_after_pf_hadron_calibration": ratios["after"],
    }

    try:
        profiles_directory = output_file.mkdir("profiles")
        ratios_directory = output_file.mkdir("ratios")
        if not profiles_directory or not ratios_directory:
            raise RuntimeError(f"Could not create directories in ROOT output file: {output_path}")

        profiles_directory.cd()
        for object_name, profile in profile_objects.items():
            if profile.Write(object_name, ROOT.TObject.kOverwrite) <= 0:
                raise RuntimeError(f"Could not write profiles/{object_name} to {output_path}")

        ratios_directory.cd()
        for object_name, ratio in ratio_objects.items():
            if ratio.Write(object_name, ROOT.TObject.kOverwrite) <= 0:
                raise RuntimeError(f"Could not write ratios/{object_name} to {output_path}")

        output_file.Write()
    finally:
        output_file.Close()

    expected_objects = {
        **{f"profiles/{name}": "TProfile" for name in profile_objects},
        **{f"ratios/{name}": "TH1D" for name in ratio_objects},
    }
    validation_file = open_root_file(output_path)
    try:
        for object_path, expected_type in expected_objects.items():
            saved_object = validation_file.Get(object_path)
            if not saved_object:
                raise RuntimeError(f"Missing '{object_path}' in ROOT output file: {output_path}")
            if not saved_object.InheritsFrom(expected_type):
                raise RuntimeError(
                    f"'{object_path}' in {output_path} is not a {expected_type}"
                )
    finally:
        validation_file.Close()

    return output_path


def configure_log_axis(pad):
    pad.SetLogx(True)
    frame = CMS.GetCmsCanvasHist(pad)
    if frame:
        frame.GetXaxis().SetMoreLogLabels(True)
        frame.GetXaxis().SetNoExponent(True)


def make_plot(data_profiles, mc_profiles, ratios, ratio_min, ratio_max, output_dir, output_name):
    if ratio_min >= ratio_max:
        raise RuntimeError("ratio-min must be smaller than ratio-max")

    ROOT.gROOT.SetBatch(ROOT.kTRUE)

    CMS.SetCmsText("")
    CMS.SetExtraText("LowPU(1-2)")
    CMS.SetLumi(None, run="2026C")
    CMS.SetEnergy(13.6)

    canvas = CMS.cmsDiCanvas(
        "pfHadronCalibrationResponseVsPDataMCRatio",
        3.0,
        100.0,
        0.6,
        1.4,
        ratio_min,
        ratio_max,
        "Track momentum p [GeV]",
        "#LT(E_{ECAL} + E_{HCAL}) / p#GT",
        "Data / MC",
        square=True,
        iPos=0,
        extraSpace=0.04,
    )
    canvas.SetCanvasSize(900, 1050)

    before_color = ROOT.TColor.GetColor("#E07B39")
    after_color = ROOT.TColor.GetColor("#3B75AF")

    upper_pad = canvas.cd(1)
    configure_log_axis(upper_pad)

    response_unity_line = ROOT.TLine(3.0, 1.0, 100.0, 1.0)
    CMS.cmsDrawLine(response_unity_line, lcolor=ROOT.kGray + 2, lstyle=ROOT.kDotted, lwidth=2)

    CMS.cmsDraw(
        mc_profiles["before"],
        "E1PL",
        marker=ROOT.kOpenSquare,
        msize=1.1,
        mcolor=before_color,
        lstyle=ROOT.kDashed,
        lwidth=2,
        lcolor=before_color,
        fstyle=0,
    )
    CMS.cmsDraw(
        mc_profiles["after"],
        "E1PL",
        marker=ROOT.kOpenSquare,
        msize=1.1,
        mcolor=after_color,
        lstyle=ROOT.kDashed,
        lwidth=2,
        lcolor=after_color,
        fstyle=0,
    )
    CMS.cmsDraw(
        data_profiles["before"],
        "E1PL",
        marker=ROOT.kFullCircle,
        msize=1.0,
        mcolor=before_color,
        lstyle=ROOT.kSolid,
        lwidth=2,
        lcolor=before_color,
        fstyle=0,
    )
    CMS.cmsDraw(
        data_profiles["after"],
        "E1PL",
        marker=ROOT.kFullCircle,
        msize=1.0,
        mcolor=after_color,
        lstyle=ROOT.kSolid,
        lwidth=2,
        lcolor=after_color,
        fstyle=0,
    )

    data_legend = CMS.cmsLeg(0.19, 0.72, 0.49, 0.88, textSize=0.031)
    data_legend.AddEntry(data_profiles["before"], "Before PFHC", "lep")
    data_legend.AddEntry(data_profiles["after"], "After PFHC", "lep")
    CMS.cmsHeader(data_legend, "Data (EphemeralZeroBias)", textSize=0.031)

    mc_legend = CMS.cmsLeg(0.59, 0.72, 0.94, 0.88, textSize=0.031)
    mc_legend.AddEntry(mc_profiles["before"], "Before PFHC", "lep")
    mc_legend.AddEntry(mc_profiles["after"], "After PFHC", "lep")
    CMS.cmsHeader(mc_legend, "MC (PionGunNoPU)", textSize=0.031)

    upper_pad.RedrawAxis()

    lower_pad = canvas.cd(2)
    configure_log_axis(lower_pad)

    ratio_unity_line = ROOT.TLine(3.0, 1.0, 100.0, 1.0)
    CMS.cmsDrawLine(ratio_unity_line, lcolor=ROOT.kGray + 2, lstyle=ROOT.kDotted, lwidth=2)
    CMS.cmsDraw(
        ratios["before"],
        "E1P",
        marker=ROOT.kFullCircle,
        msize=1.0,
        mcolor=before_color,
        lstyle=ROOT.kSolid,
        lwidth=2,
        lcolor=before_color,
        fstyle=0,
    )
    CMS.cmsDraw(
        ratios["after"],
        "E1P",
        marker=ROOT.kFullCircle,
        msize=1.0,
        mcolor=after_color,
        lstyle=ROOT.kSolid,
        lwidth=2,
        lcolor=after_color,
        fstyle=0,
    )
    lower_pad.RedrawAxis()

    output_dir.mkdir(parents=True, exist_ok=True)
    output_stem = output_dir / output_name
    pdf_path = output_stem.with_suffix(".pdf")
    png_path = output_stem.with_suffix(".png")
    CMS.SaveCanvas(canvas, str(pdf_path), close=False)
    CMS.SaveCanvas(canvas, str(png_path))
    return pdf_path, png_path


def main():
    args = parse_arguments()

    output_dir = args.output_dir.resolve()
    root_output_path = (
        args.root_output.resolve()
        if args.root_output is not None
        else output_dir / f"{args.output_name}.root"
    )

    data_profiles = load_profiles(args.data_file.resolve(), "ratio_data")
    mc_profiles = load_profiles(args.mc_file.resolve(), "ratio_mc")
    validate_common_binning([*data_profiles.values(), *mc_profiles.values()])

    ratios = {
        calibration: make_ratio_histogram(
            data_profiles[calibration], mc_profiles[calibration], f"data_mc_{calibration}_ratio"
        )
        for calibration in ("before", "after")
    }

    root_output_file = write_root_output(data_profiles, mc_profiles, ratios, root_output_path)

    output_files = make_plot(
        data_profiles,
        mc_profiles,
        ratios,
        args.ratio_min,
        args.ratio_max,
        output_dir,
        args.output_name,
    )
    for output_file in (*output_files, root_output_file):
        print(f"Wrote {output_file}")


if __name__ == "__main__":
    try:
        main()
    except RuntimeError as error:
        print(f"ERROR: {error}", file=sys.stderr)
        sys.exit(1)
