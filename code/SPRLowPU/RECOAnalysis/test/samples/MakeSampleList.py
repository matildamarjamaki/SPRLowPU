import os
import subprocess
import sys, json

def main(datasetName,sampleName, USERdataset=False):

  list_file_path = []

  if type(datasetName) == str:
    if not(USERdataset):
      out = subprocess.check_output("dasgoclient -json --query 'file dataset=%s'" %(datasetName), shell=True)
    else:
      out = subprocess.check_output("dasgoclient -json --query 'file dataset=%s instance=prod/phys03'" %(datasetName), shell=True)

    list_jsonDict = json.loads(out) # Get list of dictionary. Each element of list is for a file.
    for data in list_jsonDict:
      file_path = data["file"][0]["name"]
      list_file_path.append(file_path)
  elif type(datasetName) == list:
    print(f"Have extension datasets for sample: ({sampleName})")
    for thisDataset in datasetName:
      print(thisDataset)
      if not(USERdataset):
        out = subprocess.check_output("dasgoclient -json --query 'file dataset=%s'" %(thisDataset), shell=True)
      else:
        out = subprocess.check_output("dasgoclient -json --query 'file dataset=%s instance=prod/phys03'" %(thisDataset), shell=True)

      list_jsonDict = json.loads(out) # Get list of dictionary. Each element of list is for a file.

      for data in list_jsonDict:
        file_path = data["file"][0]["name"]
        list_file_path.append(file_path)
  #
  #
  #
  fout = f"{sampleName}.txt"
  fo = open(fout, "w")
  print(f"Making {fout} with nfiles={len(list_file_path)}")
  for file_path in list_file_path:
    fo.write(file_path+'\n')
  fo.close()

datasetDict = {
  "Data26C_LowPU1or2_EphemeralZeroBias0" :"/EphemeralZeroBias0/nbinnorj-Run2026C_PromptRECO_v1_LowPU1or2_RAWToRECO_2026_v0p1-4ae2899d6667910a95ae225f64732522/USER",
  "Data26C_LowPU1or2_EphemeralZeroBias1" :"/EphemeralZeroBias1/nbinnorj-Run2026C_PromptRECO_v1_LowPU1or2_RAWToRECO_2026_v0p1-4ae2899d6667910a95ae225f64732522/USER",
  "Data26C_LowPU1or2_EphemeralZeroBias2" :"/EphemeralZeroBias2/nbinnorj-Run2026C_PromptRECO_v1_LowPU1or2_RAWToRECO_2026_v0p1-4ae2899d6667910a95ae225f64732522/USER",
  "Data26C_LowPU1or2_EphemeralZeroBias3" :"/EphemeralZeroBias3/nbinnorj-Run2026C_PromptRECO_v1_LowPU1or2_RAWToRECO_2026_v0p1-4ae2899d6667910a95ae225f64732522/USER",
  "Data26C_LowPU1or2_EphemeralZeroBias4" :"/EphemeralZeroBias0/nbinnorj-Run2026C_PromptRECO_v1_LowPU1or2_RAWToRECO_2026_v0p1-4ae2899d6667910a95ae225f64732522/USER",
  "Data26C_LowPU1or2_EphemeralZeroBias5" :"/EphemeralZeroBias1/nbinnorj-Run2026C_PromptRECO_v1_LowPU1or2_RAWToRECO_2026_v0p1-4ae2899d6667910a95ae225f64732522/USER",
  "Data26C_LowPU1or2_EphemeralZeroBias6" :"/EphemeralZeroBias2/nbinnorj-Run2026C_PromptRECO_v1_LowPU1or2_RAWToRECO_2026_v0p1-4ae2899d6667910a95ae225f64732522/USER",
  "Data26C_LowPU1or2_EphemeralZeroBias7" :"/EphemeralZeroBias3/nbinnorj-Run2026C_PromptRECO_v1_LowPU1or2_RAWToRECO_2026_v0p1-4ae2899d6667910a95ae225f64732522/USER",
  # "Data24B_SpecialZeroBias0" : "/SpecialZeroBias0/nbinnorj-Run2024B_RECO_v1_CDEReproc_RAWToRECO_2024_v0p1-f98df8f6496d3e143bda6419c8a8fad9/USER",
  # "Data24B_SpecialZeroBias1" : "/SpecialZeroBias1/nbinnorj-Run2024B_RECO_v1_CDEReproc_RAWToRECO_2024_v0p1-f98df8f6496d3e143bda6419c8a8fad9/USER",
  # "Data24B_SpecialZeroBias2" : "/SpecialZeroBias2/nbinnorj-Run2024B_RECO_v1_CDEReproc_RAWToRECO_2024_v0p1-f98df8f6496d3e143bda6419c8a8fad9/USER",
  # "Data24B_SpecialZeroBias3" : "/SpecialZeroBias3/nbinnorj-Run2024B_RECO_v1_CDEReproc_RAWToRECO_2024_v0p1-f98df8f6496d3e143bda6419c8a8fad9/USER",
  # "Data24B_SpecialZeroBias4" : "/SpecialZeroBias4/nbinnorj-Run2024B_RECO_v1_CDEReproc_RAWToRECO_2024_v0p1-f98df8f6496d3e143bda6419c8a8fad9/USER",
  # "Data24B_SpecialZeroBias5" : "/SpecialZeroBias5/nbinnorj-Run2024B_RECO_v1_CDEReproc_RAWToRECO_2024_v0p1-f98df8f6496d3e143bda6419c8a8fad9/USER",
  # "Data24B_SpecialZeroBias6" : "/SpecialZeroBias6/nbinnorj-Run2024B_RECO_v1_CDEReproc_RAWToRECO_2024_v0p1-f98df8f6496d3e143bda6419c8a8fad9/USER",
  # "Data24B_SpecialZeroBias7" : "/SpecialZeroBias7/nbinnorj-Run2024B_RECO_v1_CDEReproc_RAWToRECO_2024_v0p1-f98df8f6496d3e143bda6419c8a8fad9/USER",
  # "Data24B_SpecialZeroBias8" : "/SpecialZeroBias8/nbinnorj-Run2024B_RECO_v1_CDEReproc_RAWToRECO_2024_v0p1-f98df8f6496d3e143bda6419c8a8fad9/USER",
  # "Data24B_SpecialZeroBias9" : "/SpecialZeroBias9/nbinnorj-Run2024B_RECO_v1_CDEReproc_RAWToRECO_2024_v0p1-f98df8f6496d3e143bda6419c8a8fad9/USER",
}


for key in datasetDict:
  datasetName=datasetDict[key]
  sampleName=key
  main(datasetName,sampleName,USERdataset=True)
  # main(datasetName,sampleName,USERdataset=False)


