cmsDriver.py step2 \
 -s NANO:@Scout \
 --process NANO \
 --eventcontent NANOAOD \
 --datatier NANOAOD \
 --data \
 --era Run3_2025 \
 --conditions 150X_dataRun3_Prompt_v1 \
 --python_file scoutingnano_data_standalone.py \
 -n 100 \
 --fileout scouting_nano.root \
 --filein root://eosuser.cern.ch//eos/user/n/nbinnorj/Samples/HLTSCOUT/store/data/Run2025E/ScoutingPF0/HLTSCOUT/v1/000/396/258/00000/fc7aa1b6-141b-4c10-a7ce-38f82e0628b6.root \
 --no_exec

