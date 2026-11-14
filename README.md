# Low-PU Particle Flow Candidate Study

MSc thesis research project studying particle-flow (PF) reconstruction
and hadronic response in low-pileup CMS data.

## Research topics

- Charged-hadron PF candidate response and data/MC comparisons.
- PFBlock and HCAL PFCluster associations.
- Neutral-hadron contributions associated with charged hadrons.
- V0 reconstruction and response studies, including K0S and Lambda candidates.

## Original CERN environment

CMSSW release: CMSSW_16_0_5_patch1

Original working directory:
`/eos/home-m/mmarjama/SPRLowPU_2026/CMSSW_16_0_5_patch1/src/SPRLowPU`

Original Git repository:
`https://github.com/nurfikri89/SPRLowPU`

Working fork:
`https://github.com/matildamarjamaki/SPRLowPU`

Working branch at snapshot:
`feature/v0-pf-response`

Last committed source revision at snapshot:
`7d85963`

Important: the original working tree contained uncommitted changes.
The files in this repository are a working-tree snapshot, not a clean
checkout of commit 7d85963.

## Repository contents

- `code/SPRLowPU/RECOAnalysis/plugins/`: CMSSW analyzers and build configuration.
- `code/SPRLowPU/RECOAnalysis/python/`: Python modules and configurations.
- `code/SPRLowPU/RECOAnalysis/test/`: analysis configurations and test scripts.
- `code/SPRLowPU/RECOAnalysis/plotting/`: plotting and response studies.
- `configs/condor/`: selected HTCondor production scripts.

Generated ROOT outputs, build products, large sample inventories,
and temporary backups are not included in version control.

## Restoring the analysis

The source package must be placed at:

`$CMSSW_BASE/src/SPRLowPU/RECOAnalysis`

The intended environment is CMSSW_16_0_5_patch1.

A fresh CMSSW build and test run have not yet been validated from
this repository. Dataset access, additional dependencies, and
production configurations must be checked before claiming complete
reproducibility.

## Provenance

This repository contains a curated snapshot of work based on the
SPRLowPU project developed by Nurfikri Norjoharuddeen and collaborators.

The original upstream repository and working fork are retained above
for attribution and comparison.
