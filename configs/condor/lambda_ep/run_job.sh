#!/bin/bash
set -euo pipefail

PART="$1"

CMSSW_SRC="/eos/home-m/mmarjama/SPRLowPU_2026/CMSSW_16_0_5_patch1/src"
OUTPUT_DIR="/eos/home-m/mmarjama/SPRLowPU_2026/lambda_ep_full/results"

source /cvmfs/cms.cern.ch/cmsset_default.sh

cd "$CMSSW_SRC"
eval "$(scram runtime -sh)"

WORKDIR="$(mktemp -d)"
trap 'rm -rf "$WORKDIR"' EXIT

OUTPUT="$WORKDIR/spr_lambda_ep_part${PART}.root"

echo "Running part ${PART}"
echo "Host: $(hostname)"
echo "Output: $OUTPUT"

cmsRun SPRLowPU/RECOAnalysis/test/runSPRLowPUV0Analyzer_batch_cfg.py \
  sampleName="Data26C_LowPU1or2_EphemeralZeroBias0_part${PART}" \
  outputFile="$OUTPUT" \
  maxEvents=-1

test -s "$OUTPUT"

cp "$OUTPUT" "$OUTPUT_DIR/spr_lambda_ep_part${PART}.root"

echo "Completed part ${PART}"
