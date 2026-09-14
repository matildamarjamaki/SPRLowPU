#!/usr/bin/env python3

import argparse
import sys
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

ANALYZER_DIRECTORY = "sprLowPUDataAnalyzer"
PRE_CALIBRATION_PROFILE = "meanPrePFHadronCalibrationLinkedECALPlusHCALResponseVsPVarBins"
POST_CALIBRATION_PROFILE = "meanPFHadronCalibratedLinkedECALPlusHCALResponseVsPVarBins"


def parse_arguments():
    parser = argparse.ArgumentParser(
        description="Compare the PF-cluster response before and after PF hadron calibration in Data and MC."
    )
    parser.add_argument("--data-file", type=Path, default=DEFAULT_DATA_FILE, help="Merged Data ROOT file")
    parser.add_argument("--mc-file", type=Path, default=DEFAULT_MC_FILE, help="Merged MC ROOT file")
    parser.add_argument(
        "--output-dir", type=Path, default=DEFAULT_OUTPUT_DIR, help="Directory in which plots are written"
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


def make_plot(data_profiles, mc_profiles, output_dir):
    ROOT.gROOT.SetBatch(ROOT.kTRUE)

    CMS.SetCmsText("")
    CMS.SetExtraText("LowPU(1-2)")
    CMS.SetLumi(None, run="2026C")
    CMS.SetEnergy(13.6)

    canvas = CMS.cmsCanvas(
        "pfHadronCalibrationResponseVsPDataMC",
        3.0,
        100.0,
        0.6,
        1.4,
        "Track momentum p [GeV]",
        "#LT(E_{ECAL} + E_{HCAL}) / p#GT",
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

    unity_line = ROOT.TLine(3.0, 1.0, 100.0, 1.0)
    CMS.cmsDrawLine(unity_line, lcolor=ROOT.kGray + 2, lstyle=ROOT.kDotted, lwidth=2)

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

    data_legend = CMS.cmsLeg(0.19, 0.74, 0.49, 0.88, textSize=0.031)
    data_legend.AddEntry(data_profiles["before"], "Before PFHC", "lep")
    data_legend.AddEntry(data_profiles["after"], "After PFHC", "lep")
    CMS.cmsHeader(data_legend, "Data (EphemeralZeroBias)", textSize=0.031)

    mc_legend = CMS.cmsLeg(0.59, 0.74, 0.94, 0.88, textSize=0.031)
    mc_legend.AddEntry(mc_profiles["before"], "Before PFHC", "lep")
    mc_legend.AddEntry(mc_profiles["after"], "After PFHC", "lep")
    CMS.cmsHeader(mc_legend, "MC (PionGunNoPU)", textSize=0.031)

    canvas.RedrawAxis()

    output_dir.mkdir(parents=True, exist_ok=True)
    output_stem = output_dir / "pf_hadron_calibration_response_vs_p_data_mc"
    CMS.SaveCanvas(canvas, str(output_stem.with_suffix(".pdf")), close=False)
    CMS.SaveCanvas(canvas, str(output_stem.with_suffix(".png")))

    return output_stem.with_suffix(".pdf"), output_stem.with_suffix(".png")


def main():
    args = parse_arguments()

    data_profiles = load_profiles(args.data_file.resolve(), "data")
    mc_profiles = load_profiles(args.mc_file.resolve(), "mc")
    validate_common_binning([*data_profiles.values(), *mc_profiles.values()])

    output_files = make_plot(data_profiles, mc_profiles, args.output_dir.resolve())
    for output_file in output_files:
        print(f"Wrote {output_file}")


if __name__ == "__main__":
    try:
        main()
    except RuntimeError as error:
        print(f"ERROR: {error}", file=sys.stderr)
        sys.exit(1)
