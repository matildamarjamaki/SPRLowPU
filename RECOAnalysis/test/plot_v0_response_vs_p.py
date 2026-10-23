import ROOT
from pathlib import Path

ROOT.gROOT.SetBatch(True)
ROOT.gStyle.SetOptStat(0)

filename = "/tmp/mmarjama/spr_v0_test/sprLowPU_v0_loosecuts.root"
outdir = Path(
    "/eos/home-m/mmarjama/SPRLowPU_2026/results/v0_pf_response"
)
outdir.mkdir(parents=True, exist_ok=True)

f = ROOT.TFile.Open(filename)
assert f and not f.IsZombie()

d = f.Get("sprLowPUDataAnalyzer")
assert d

species = [
    ("Lambda proton", "v0LambdaProtonResponseVsP", ROOT.kBlue + 1),
    ("anti-Lambda antiproton", "v0AntiLambdaAntiprotonResponseVsP", ROOT.kRed + 1),
    ("Lambda pion", "v0LambdaPionResponseVsP", ROOT.kGreen + 2),
    ("Kshort pion", "v0KshortPionResponseVsP", ROOT.kMagenta + 1),
]

canvas = ROOT.TCanvas("c", "", 950, 700)
canvas.SetLeftMargin(0.13)
canvas.SetBottomMargin(0.13)

legend = ROOT.TLegend(0.53, 0.65, 0.89, 0.88)
legend.SetBorderSize(0)
legend.SetFillStyle(0)

profiles = []

for name, histname, color in species:
    p = d.Get(histname)
    assert p, histname

    p.SetLineColor(color)
    p.SetMarkerColor(color)
    p.SetMarkerStyle(20)
    p.SetMarkerSize(0.9)
    p.SetLineWidth(2)

    profiles.append((name, p))

    print(f"\n{name}: {int(p.GetEntries())} entries")
    print(f"{'p bin [GeV]':22s} {'N':>8s} {'Mean E/p':>12s}")

    for i in range(1, p.GetNbinsX() + 1):
        n = p.GetBinEntries(i)
        if n == 0:
            continue

        low = p.GetXaxis().GetBinLowEdge(i)
        high = p.GetXaxis().GetBinUpEdge(i)

        print(
            f"{low:8.2f} - {high:8.2f}"
            f"{n:8.0f}"
            f"{p.GetBinContent(i):12.4f}"
        )

frame = canvas.DrawFrame(1.0, 0.0, 10.0, 1.5)
frame.SetTitle("PF response by V0 species hypothesis")
frame.GetXaxis().SetTitle("Track momentum p [GeV]")
frame.GetYaxis().SetTitle(
    "<(E_{ECAL}^{PF} + E_{HCAL}^{PF}) / p_{track}>"
)

for name, p in profiles:
    p.Draw("E1 SAME")
    legend.AddEntry(p, name, "lep")

legend.Draw()

canvas.SaveAs(str(outdir / "v0_response_vs_p.png"))
canvas.SaveAs(str(outdir / "v0_response_vs_p.pdf"))

f.Close()
print("\nSaved to:", outdir)
