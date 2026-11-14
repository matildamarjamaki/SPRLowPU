
import ROOT

ROOT.gROOT.SetBatch(True)
ROOT.gStyle.SetOptStat(0)
ROOT.gStyle.SetPalette(ROOT.kViridis)

input_file = "/afs/cern.ch/user/m/mmarjama/CMSSW_15_0_15_patch4/histo_all_simtrack.root"
output_pdf = "/afs/cern.ch/user/m/mmarjama/CMSSW_15_0_15_patch4/reco_analysis_plots_simtrack.pdf"

f = ROOT.TFile.Open(input_file)
if not f or f.IsZombie():
    raise RuntimeError(f"Could not open {input_file}")

d = f.Get("recoAnalyzer")
if not d:
    raise RuntimeError("Directory recoAnalyzer not found")


def get(name):
    obj = d.Get(name)
    if not obj:
        raise RuntimeError(f"Histogram {name} not found")
    return obj


def title(text):
    latex = ROOT.TLatex()
    latex.SetNDC()
    latex.SetTextSize(0.045)
    latex.DrawLatex(0.12, 0.92, text)


def make_eff(num_name, den_name, name, hist_title):
    num = get(num_name)
    den = get(den_name)

    eff = num.Clone(name)
    eff.Reset()
    eff.Divide(num, den, 1.0, 1.0, "B")
    eff.SetTitle(hist_title)
    eff.SetLineWidth(2)
    eff.SetMarkerStyle(20)
    eff.SetMarkerSize(0.8)
    eff.GetYaxis().SetRangeUser(0.0, 1.1)
    return eff


def draw_hist(name, drawopt="HIST", logy=False, logz=False):
    ROOT.gPad.SetLogy(logy)
    ROOT.gPad.SetLogz(logz)
    h = get(name)
    h.SetLineWidth(2)
    h.Draw(drawopt)
    return h


def draw_projection_z_from_h3(h3_name, x_min, x_max, proj_name):
    h3 = get(h3_name)

    xbin_min = h3.GetXaxis().FindBin(x_min)
    xbin_max = h3.GetXaxis().FindBin(x_max) - 1

    h = h3.ProjectionZ(
        proj_name,
        xbin_min,
        xbin_max,
        1,
        h3.GetYaxis().GetNbins()
    )

    h.SetTitle(";(ECAL+HCAL)/p;tracks")
    h.SetLineWidth(2)
    h.Draw("HIST")
    return h


energy_bins = [
    (1, 5),
    (5, 10),
    (10, 20),
    (20, 30),
    (30, 50),
    (50, 75),
    (75, 100),
    (100, 200),
]


c = ROOT.TCanvas("c", "c", 1700, 1200)
c.Print(output_pdf + "[")

# Page 1: main response overview
c.Clear()
c.Divide(2, 2)

c.cd(1)
draw_hist("h_ep_total_barrel")
title("Barrel: PFBlock cluster-level (ECAL+HCAL)/p")

c.cd(2)
draw_hist("h_ep_total_endcap")
title("Endcap: PFBlock cluster-level (ECAL+HCAL)/p")

c.cd(3)
draw_hist("h2_ep_total_vs_genE_barrel", "COLZ", logz=True)
title("Barrel: response vs gen pion energy")

c.cd(4)
draw_hist("h2_ep_total_vs_genE_endcap", "COLZ", logz=True)
title("Endcap: response vs gen pion energy")

c.Print(output_pdf)

# Page 2: gen pion energy and eta distributions
c.Clear()
c.Divide(2, 2)

c.cd(1)
draw_hist("h_gen_energy_den")
title("Gen pion energy denominator")

c.cd(2)
draw_hist("h_gen_eta_den")
title("Gen pion eta denominator")

c.cd(3)
draw_hist("h2_gen_eta_vs_energy", "COLZ", logz=True)
title("Gen pion eta vs energy")

c.cd(4)
draw_hist("h_min_dr_gen_pftrack")
title("Minimum DeltaR(gen pion, PF track)")

c.Print(output_pdf)


# Page 3: gen pion -> SimTrack -> reco track -> HCAL PFBlock chain
eff_simtrack = make_eff(
    "h_gen_energy_num_simtrack",
    "h_gen_energy_den_trackeff",
    "eff_simtrack",
    ";gen pion energy [GeV];efficiency"
)

eff_simtrack_recotrack = make_eff(
    "h_gen_energy_num_simtrack_recotrack",
    "h_gen_energy_den_trackeff",
    "eff_simtrack_recotrack",
    ";gen pion energy [GeV];efficiency"
)

eff_simtrack_recotrack_hcal = make_eff(
    "h_gen_energy_num_simtrack_recotrack_hcalblock",
    "h_gen_energy_den_trackeff",
    "eff_simtrack_recotrack_hcal",
    ";gen pion energy [GeV];efficiency"
)

c.Clear()
c.Divide(2, 2)

c.cd(1)
eff_simtrack.Draw("E")
title("gen pion -> SimTrack")

c.cd(2)
eff_simtrack_recotrack.Draw("E")
title("gen pion -> SimTrack -> reco track")

c.cd(3)
eff_simtrack_recotrack_hcal.Draw("E")
title("gen pion -> SimTrack -> reco track -> HCAL PFBlock")

c.cd(4)
draw_hist("h_gen_energy_den_trackeff")
title("Denominator: gen pion energy")

c.Print(output_pdf)


# Page 3: full eta efficiencies vs gen pion energy
eff_track_E = make_eff(
    "h_gen_energy_num_trackeff",
    "h_gen_energy_den_trackeff",
    "eff_track_E",
    ";gen pion energy [GeV];efficiency"
)

eff_hcal_E = make_eff(
    "h_gen_energy_num_track_hcalblock",
    "h_gen_energy_den_trackeff",
    "eff_hcal_E",
    ";gen pion energy [GeV];efficiency"
)

c.Clear()
c.Divide(2, 2)

c.cd(1)
draw_hist("h_gen_energy_den_trackeff")
title("Full eta: denominator")

c.cd(2)
draw_hist("h_gen_energy_num_trackeff")
title("Full eta: matched reco track numerator")

c.cd(3)
eff_track_E.Draw("E")
title("Full eta: gen pion -> reco track")

c.cd(4)
eff_hcal_E.Draw("E")
title("Full eta: gen pion -> track in HCAL PFBlock")

c.Print(output_pdf)

# Page 4: barrel efficiencies vs gen pion energy
eff_track_E_barrel = make_eff(
    "h_gen_energy_num_trackeff_barrel",
    "h_gen_energy_den_trackeff_barrel",
    "eff_track_E_barrel",
    ";gen pion energy [GeV];efficiency"
)

eff_hcal_E_barrel = make_eff(
    "h_gen_energy_num_track_hcalblock_barrel",
    "h_gen_energy_den_trackeff_barrel",
    "eff_hcal_E_barrel",
    ";gen pion energy [GeV];efficiency"
)

c.Clear()
c.Divide(2, 2)

c.cd(1)
draw_hist("h_gen_energy_den_trackeff_barrel")
title("Barrel: denominator")

c.cd(2)
draw_hist("h_gen_energy_num_trackeff_barrel")
title("Barrel: matched reco track numerator")

c.cd(3)
eff_track_E_barrel.Draw("E")
title("Barrel: gen pion -> reco track")

c.cd(4)
eff_hcal_E_barrel.Draw("E")
title("Barrel: gen pion -> track in HCAL PFBlock")

c.Print(output_pdf)

# Page 5: endcap efficiencies vs gen pion energy
eff_track_E_endcap = make_eff(
    "h_gen_energy_num_trackeff_endcap",
    "h_gen_energy_den_trackeff_endcap",
    "eff_track_E_endcap",
    ";gen pion energy [GeV];efficiency"
)

eff_hcal_E_endcap = make_eff(
    "h_gen_energy_num_track_hcalblock_endcap",
    "h_gen_energy_den_trackeff_endcap",
    "eff_hcal_E_endcap",
    ";gen pion energy [GeV];efficiency"
)

c.Clear()
c.Divide(2, 2)

c.cd(1)
draw_hist("h_gen_energy_den_trackeff_endcap")
title("Endcap: denominator")

c.cd(2)
draw_hist("h_gen_energy_num_trackeff_endcap")
title("Endcap: matched reco track numerator")

c.cd(3)
eff_track_E_endcap.Draw("E")
title("Endcap: gen pion -> reco track")

c.cd(4)
eff_hcal_E_endcap.Draw("E")
title("Endcap: gen pion -> track in HCAL PFBlock")

c.Print(output_pdf)

# Page 6: eta efficiencies
eff_track_eta = make_eff(
    "h_gen_eta_num_trackeff",
    "h_gen_eta_den_trackeff",
    "eff_track_eta",
    ";gen pion eta;efficiency"
)

eff_hcal_eta = make_eff(
    "h_gen_eta_num_track_hcalblock",
    "h_gen_eta_den_trackeff",
    "eff_hcal_eta",
    ";gen pion eta;efficiency"
)

c.Clear()
c.Divide(2, 2)

c.cd(1)
draw_hist("h_gen_eta_den_trackeff")
title("Denominator: gen pion eta")

c.cd(2)
draw_hist("h_gen_eta_num_trackeff")
title("Numerator: matched reco track")

c.cd(3)
eff_track_eta.Draw("E")
title("Tracking efficiency vs eta")

c.cd(4)
eff_hcal_eta.Draw("E")
title("Track in HCAL PFBlock efficiency vs eta")

c.Print(output_pdf)

# Page 7: barrel energy-bin matrix
c.Clear()
c.Divide(4, 2)

for i, (emin, emax) in enumerate(energy_bins, start=1):
    c.cd(i)
    ROOT.gPad.SetLogy(False)
    ROOT.gPad.SetLogz(False)

    draw_projection_z_from_h3(
        "h3_ep_total_genE_eta_barrel",
        emin,
        emax,
        f"h_ep_barrel_genE_{emin}_{emax}"
    )

    title(f"Barrel, {emin} < gen E < {emax} GeV")

c.Print(output_pdf)

# Page 8: endcap energy-bin matrix
c.Clear()
c.Divide(4, 2)

for i, (emin, emax) in enumerate(energy_bins, start=1):
    c.cd(i)
    ROOT.gPad.SetLogy(False)
    ROOT.gPad.SetLogz(False)

    draw_projection_z_from_h3(
        "h3_ep_total_genE_eta_endcap",
        emin,
        emax,
        f"h_ep_endcap_genE_{emin}_{emax}"
    )

    title(f"Endcap, {emin} < gen E < {emax} GeV")

c.Print(output_pdf)

# Page 9: matched/unmatched and response cross-checks
c.Clear()
c.Divide(2, 2)

c.cd(1)
draw_hist("h_ep_total_matched_dr03")
title("Matched: DeltaR < 0.3")

c.cd(2)
draw_hist("h_ep_total_unmatched_dr03")
title("Unmatched: DeltaR >= 0.3")

c.cd(3)
draw_hist("h2_ep_total_vs_genE_barrel", "COLZ", logz=True)
title("Barrel: response vs gen pion energy")

c.cd(4)
draw_hist("h2_ep_total_vs_genE_endcap", "COLZ", logz=True)
title("Endcap: response vs gen pion energy")

c.Print(output_pdf)

c.Print(output_pdf + "]")

print(f"Saved {output_pdf}")