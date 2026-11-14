# Low-PU PF Candidate Study — Project Status

Updated: 2026-11-14

## Research question

Study particle-flow candidate response and reconstruction
in low-pileup CMS data, including charged hadrons, neutral
energy associations, and reconstructed V0 candidates.

## Research components

1. Charged-hadron PF candidate response and data/MC studies.
2. PFBlock and HCAL PFCluster associations.
3. Neutral-hadron contributions associated with charged hadrons.
4. K0S and Lambda V0-tagged response studies.

## Saved implementation

- `SPRLowPUDataAnalyzer.cc`
- `SPRLowPUV0Analyzer.cc`
- CMSSW Python configurations and analysis scripts.
- Selected plotting scripts.
- Selected HTCondor Lambda production configurations.

The saved source is a snapshot of the original CERN working
tree, including changes that had not been committed there.

## Original CERN environment

CMSSW release:
`CMSSW_16_0_5_patch1`

Original working directory:
`/eos/home-m/mmarjama/SPRLowPU_2026/CMSSW_16_0_5_patch1/src/SPRLowPU`

Original development branch:
`feature/v0-pf-response`

Original branch HEAD at snapshot:
`7d85963`

## Important local files

- `README.md`
- `code/SPRLowPU/RECOAnalysis/plugins/`
- `code/SPRLowPU/RECOAnalysis/test/`
- `code/SPRLowPU/RECOAnalysis/plotting/`
- `configs/condor/`

## Outstanding work

1. Check the current CERN production and output status.
2. Inventory the important Lambda and PFBlock ROOT outputs.
3. Validate response histograms and reconstruction selections.
4. Complete the intended PFBlock/HCAL association studies.
5. Document data/MC comparisons and uncertainties.
6. Test a fresh CMSSW build from the saved source snapshot.

## Current limitations

The snapshot has not yet been independently built and tested
in a fresh CMSSW environment.

Large ROOT outputs and complete dataset inventories are not
included in GitHub.

## GitHub

Repository:
https://github.com/matildamarjamaki/SPRLowPU

Snapshot branch:
`thesis/lowpu-snapshot-2026`

Saved commit:
`5f71179`

Do not merge the independent snapshot history directly into
the original SPRLowPU main branch.
