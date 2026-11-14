import ROOT
from pathlib import Path

ROOT.gROOT.SetBatch(True)
ROOT.gStyle.SetOptStat(0)

outdir = Path(
    "/eos/home-m/mmarjama/SPRLowPU_2026/results/v0_pf_response"
)
f = ROOT.TFile.Open(str(outdir / "sprLowPU_v0_positive_response.root"))
assert f and not f.IsZombie()

d = f.Get("sprLowPUDataAnalyzer")
assert d

species = [
    ("Lambda proton",
     "v0LambdaProtonResponseVsP",
     "v0LambdaProtonPositiveResponseVsP",
     "lambda_proton"),
    ("anti-Lambda antiproton",
     "v0AntiLambdaAntiprotonResponseVsP",
     "v0AntiLambdaAntiprotonPositiveResponseVsP",
     "antilambda_antiproton"),
    ("Lambda pion",
     "v0LambdaPionResponseVsP",
     "v0LambdaPionPositiveResponseVsP",
     "lambda_pion"),
    ("Kshort pion",
     "v0KshortPionResponseVsP",
     "v0KshortPionPositiveResponseVsP",
     "kshort_pion"),
]

for title, all_name, pos_name, suffix in species:
    h_all = d.Get(all_name)
    h_pos = d.Get(pos_name)
    assert h_all and h_pos

    c = ROOT.TCanvas("c_" + suffix, "", 900, 650)
    c.SetLeftMargin(0.13)
    c.SetBottomMargin(0.13)
    c.SetGrid()

    frame = c.DrawFrame(1.0, 0.0, 7.5, 1.6)
    frame.SetTitle(
        f"{title};Track momentum p [GeV];Mean PF (ECAL+HCAL)/p"
    )

    graphs = []

    for h, color, marker, min_entries in [
        (h_all, ROOT.kBlue + 1, 20, 1),
        (h_pos, ROOT.kRed + 1, 21, 3),
    ]:
        g = ROOT.TGraphErrors()

        for i in range(1, h.GetNbinsX() + 1):
            n = h.GetBinEntries(i)
            if n < min_entries:
                continue

            x = h.GetXaxis().GetBinCenter(i)
            if x > 7.5:
                continue

            j = g.GetN()
            g.SetPoint(j, x, h.GetBinContent(i))
            g.SetPointError(
                j,
                0.0,
                h.GetBinError(i)
            )

        g.SetMarkerStyle(marker)
        g.SetMarkerSize(1.1)
        g.SetMarkerColor(color)
        g.SetLineColor(color)
        g.Draw("P SAME")
        graphs.append(g)

    legend = ROOT.TLegend(0.48, 0.75, 0.87, 0.88)
    legend.SetBorderSize(0)
    legend.AddEntry(graphs[0], "All PF matches", "p")
    legend.AddEntry(graphs[1], "PF E/p > 0 (N #geq 3/bin)", "p")
    legend.Draw()

    c.SaveAs(str(outdir / f"v0_response_comparison_{suffix}.pdf"))
    c.SaveAs(str(outdir / f"v0_response_comparison_{suffix}.png"))

    print(f"Saved: v0_response_comparison_{suffix}.png")

f.Close()
