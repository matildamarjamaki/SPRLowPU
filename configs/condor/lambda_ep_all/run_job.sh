#!/bin/bash
set -euo pipefail

DATASET="$1"
PART="$2"

CMSSW_SRC="/eos/home-m/mmarjama/SPRLowPU_2026/CMSSW_16_0_5_patch1/src"
BASE="/eos/home-m/mmarjama/SPRLowPU_2026/lambda_ep_all"
OUTPUT_DIR="$BASE/EphemeralZeroBias${DATASET}/results"

source /cvmfs/cms.cern.ch/cmsset_default.sh

cd "$CMSSW_SRC"
eval "$(scram runtime -sh)"

mkdir -p "$OUTPUT_DIR"

WORKDIR="$(mktemp -d)"
trap 'rm -rf "$WORKDIR"' EXIT

OUTPUT="$WORKDIR/spr_lambda_ep_part${PART}.root"
DEST="$OUTPUT_DIR/spr_lambda_ep_part${PART}.root"

echo "Dataset: EphemeralZeroBias${DATASET}"
echo "Part: ${PART}"
echo "Host: $(hostname)"

cmsRun SPRLowPU/RECOAnalysis/test/runSPRLowPUV0Analyzer_batch_cfg.py \
    sampleName="Data26C_LowPU1or2_EphemeralZeroBias${DATASET}_part${PART}" \
    outputFile="$OUTPUT" \
    maxEvents=-1

test -s "$OUTPUT"

if [ -e "$DEST" ]; then
    echo "ERROR: Output already exists: $DEST"
    exit 1
fi

cp "$OUTPUT" "$DEST"

echo "Completed EphemeralZeroBias${DATASET}, part ${PART}"
