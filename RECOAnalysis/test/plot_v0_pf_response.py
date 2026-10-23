import ROOT
from pathlib import Path

ROOT.gROOT.SetBatch(True)
ROOT.gStyle.SetOptStat(0)

input_file = (
    "/tmp/mmarjama/spr_v0_test/"
    "sprLowPU_v0_loosecuts.root"
)

outdir = Path(
    "/eos/home-m/mmarjama/SPRLowPU_2026/"
    "results/v0_pf_response"
)
outdir.mkdir(parents=True, exist_ok=True)

f = ROOT.TFile.Open(input_file)
assert f and not f.IsZombie(), input_file

d = f.Get("sprLowPUDataAnalyzer")
assert d

h2 = d.Get("v0SpeciesPFResponse")
assert h2 and h2.InheritsFrom("TH2"), (
    "v0SpeciesPFResponse is missing or not TH2"
)

counts = d.Get("v0SpeciesCounts")
matches = d.Get("v0SpeciesPFMatches")
assert counts and matches

species = [
    ("Lambda proton", "lambda_proton", ROOT.kBlue + 1),
    ("anti-Lambda antiproton", "antilambda_antiproton", ROOT.kRed + 1),
    ("Lambda pion", "lambda_pion", ROOT.kGreen + 2),
    ("Kshort pion", "kshort_pion", ROOT.kMagenta + 1),
]

# Detect which axis represents species.
def is_species_axis(axis):
    return (
        axis.GetNbins() == 4
        and axis.GetXmin() <= 1
        and axis.GetXmax() >= 4
    )

species_on_x = is_species_axis(h2.GetXaxis())
species_on_y = is_species_axis(h2.GetYaxis())

assert species_on_x != species_on_y, (
    "Cannot determine species axis; inspect TH2 axes"
)

print("Species axis:", "X" if species_on_x else "Y")
print("Processed events:", int(d.Get("eventCount").GetEntries()))

histograms = []

for i, (name, slug, color) in enumerate(species, 1):
    if species_on_x:
        h = h2.ProjectionY(f"response_{slug}", i, i)
    else:
        h = h2.ProjectionX(f"response_{slug}", i, i)

    h.SetDirectory(0)
    h.SetLineColor(color)
    h.SetLineWidth(2)
    h.SetMarkerColor(color)

    selected = int(counts.GetBinContent(i))
    matched = int(matches.GetBinContent(i))

    print(
        f"{name:26s} selected={selected:5d} "
        f"PF matched={matched:5d} "
        f"response entries={h.GetEntries():5.0f} "
        f"mean E/p={h.GetMean():.4f}"
    )

    c = ROOT.TCanvas(f"canvas_{slug}", "", 850, 650)
    c.SetLeftMargin(0.13)
    c.SetBottomMargin(0.13)

    h.SetTitle(f"{name} (PF matched: {matched})")
    h.GetXaxis().SetTitle(
        "(E_{ECAL}^{PF} + E_{HCAL}^{PF}) / p_{track}"
    )
    h.GetYaxis().SetTitle("Candidate hypotheses / bin")
    h.GetXaxis().SetTitleSize(0.045)
    h.GetYaxis().SetTitleSize(0.045)

    h.Draw("HIST")

    c.SaveAs(str(outdir / f"{slug}_response.png"))
    c.SaveAs(str(outdir / f"{slug}_response.pdf"))

    histograms.append((name, h))

# Combined comparison: normalize to unit area.
c = ROOT.TCanvas("comparison", "", 950, 700)
c.SetLeftMargin(0.13)
c.SetBottomMargin(0.13)

legend = ROOT.TLegend(0.52, 0.63, 0.89, 0.88)
legend.SetBorderSize(0)
legend.SetFillStyle(0)

normalized = []

for name, original in histograms:
    h = original.Clone(original.GetName() + "_normalized")
    h.SetDirectory(0)

    integral = h.Integral(1, h.GetNbinsX())

    if integral > 0:
        h.Scale(1.0 / integral)

    normalized.append((name, h))

max_y = max(h.GetMaximum() for _, h in normalized)

for index, (name, h) in enumerate(normalized):
    h.SetTitle("PF response by V0 species hypothesis")
    h.GetXaxis().SetTitle(
        "(E_{ECAL}^{PF} + E_{HCAL}^{PF}) / p_{track}"
    )
    h.GetYaxis().SetTitle("Fraction of candidate hypotheses / bin")
    h.SetMaximum(max_y * 1.25)

    h.Draw("HIST" if index == 0 else "HIST SAME")
    legend.AddEntry(h, name, "l")

legend.Draw()

c.SaveAs(str(outdir / "v0_species_response_comparison.png"))
c.SaveAs(str(outdir / "v0_species_response_comparison.pdf"))

f.Close()

print("\nPlots saved to:", outdir)
