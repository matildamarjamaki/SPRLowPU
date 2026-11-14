#
# You need to load rucio to run this script
# > source /cvmfs/cms.cern.ch/rucio/setup-py3.sh
#
import os
import json
import math
import subprocess
import sys
import time
import re
import random
from collections import defaultdict

# for data location bias
from rucio.common.utils import extract_scope
from rucio.client import Client

rucio_client = Client()

sites_whitelist = None
sites_blacklist = None
sites_regex = None
scope="cms"

def get_xrootd_sites_map():
  sites_xrootd_access = defaultdict(dict)
  # Check if the cache file has been modified in the last 10 minutes
  cache_valid = False
  if os.path.exists(".sites_map.json"):
    file_time = os.path.getmtime(".sites_map.json")
    current_time = time.time()
    ten_minutes_ago = current_time - 600
    if file_time > ten_minutes_ago:
      cache_valid = True

  if not os.path.exists(".sites_map.json") or not cache_valid:
    print("Loading SITECONF info")
    sites = [
        (s, "/cvmfs/cms.cern.ch/SITECONF/" + s + "/storage.json")
        for s in os.listdir("/cvmfs/cms.cern.ch/SITECONF/")
        if s.startswith("T")
    ]
    for site_name, conf in sites:
      if not os.path.exists(conf):
          continue
      try:
          data = json.load(open(conf))
      except Exception:
          continue
      for site in data:
          # print("site")
          # print(site)
          if site["type"] != "DISK":
              continue
          if "rse" not in site:
              continue
          if site["rse"] is None:
              continue
          for proc in site["protocols"]:
              if proc["protocol"] == "XRootD":
                  if proc["access"] not in ["global-ro", "global-rw"]:
                      continue
                  if "prefix" not in proc:
                      if "rules" in proc:
                          for rule in proc["rules"]:
                              sites_xrootd_access[site["rse"]][rule["lfn"]] = (
                                  rule["pfn"]
                              )
                  else:
                      sites_xrootd_access[site["rse"]] = proc["prefix"]
    json.dump(sites_xrootd_access, open(".sites_map.json", "w"))
  return json.load(open(".sites_map.json"))

def _get_pfn_for_site(path, rules):
  """
  Utility function that converts the file path to a valid pfn matching
  the file path with the site rules (regexes).
  """
  if isinstance(rules, dict):
    for rule, pfn in rules.items():
      if m := re.match(rule, path):
        grs = m.groups()
        for i in range(len(grs)):
          pfn = pfn.replace(f"${i+1}", grs[i])
        return pfn
  else:
    # not adding any slash as the path usually starts with it
    return rules + "/" + path.removeprefix("/")


def get_dataset_files_replicas(
  dataset,
  allowlist_sites=None,
  blocklist_sites=None,
  regex_sites=None,
  mode="full",
  partial_allowed=False,
  client=None,
  scope="cms"):

  sites_xrootd_prefix = get_xrootd_sites_map()
  client = client if client else get_rucio_client()
  outsites = []
  outfiles = []
  for filedata in client.list_replicas([{"scope": scope, "name": dataset}]):
      outfile = []
      outsite = []
      rses = filedata["rses"]
      found = False
      if allowlist_sites:
          for site in allowlist_sites:
              if site in rses:
                  # Check actual availability
                  meta = filedata["pfns"][rses[site][0]]
                  if (
                      meta["type"] != "DISK"
                      or meta["volatile"]
                      or filedata["states"][site] != "AVAILABLE"
                      or site not in sites_xrootd_prefix
                  ):
                      continue
                  outfile.append(
                      _get_pfn_for_site(filedata["name"], sites_xrootd_prefix[site])
                  )
                  outsite.append(site)
                  found = True

          if not found and not partial_allowed:
              raise Exception(
                  f"No SITE available in the allowlist for file {filedata['name']}"
              )
      else:
          possible_sites = list(rses.keys())
          if blocklist_sites:
              possible_sites = list(
                  filter(lambda key: key not in blocklist_sites, possible_sites)
              )

          if len(possible_sites) == 0 and not partial_allowed:
              raise Exception(f"No SITE available for file {filedata['name']}")

          # now check for regex
          for site in possible_sites:
              if regex_sites:
                  if re.search(regex_sites, site):
                      # Check actual availability
                      meta = filedata["pfns"][rses[site][0]]
                      if (
                          meta["type"] != "DISK"
                          or meta["volatile"]
                          or filedata["states"][site] != "AVAILABLE"
                          or site not in sites_xrootd_prefix
                      ):
                          continue
                      outfile.append(
                          _get_pfn_for_site(
                              filedata["name"], sites_xrootd_prefix[site]
                          )
                      )
                      outsite.append(site)
                      found = True
              else:
                  # Just take the first one
                  # Check actual availability
                  meta = filedata["pfns"][rses[site][0]]
                  if (
                      meta["type"] != "DISK"
                      or meta["volatile"]
                      or filedata["states"][site] != "AVAILABLE"
                      or site not in sites_xrootd_prefix
                  ):
                      continue
                  outfile.append(
                      _get_pfn_for_site(filedata["name"], sites_xrootd_prefix[site])
                  )
                  outsite.append(site)
                  found = True

      if not found and not partial_allowed:
          raise Exception(f"No SITE available for file {filedata['name']}")
      else:
          if mode == "full":
              outfiles.append(outfile)
              outsites.append(outsite)
          elif mode == "first":
              outfiles.append(outfile[0])
              outsites.append(outsite[0])
          else:
              raise NotImplementedError(f"Mode {mode} not yet implemented!")

  # Computing replicas by site:
  sites_counts = defaultdict(int)
  if mode == "full":
      for sites_by_file in outsites:
          for site in sites_by_file:
              sites_counts[site] += 1
  elif mode == "first":
      for site_by_file in outsites:
          sites_counts[site] += 1

  return outfiles, outsites, sites_counts

def do_list_sites_from_file_replicas(args) : # self, args): # [--ruciolfnse=<lfn>] [--rucioblockse=<block>] [--ruciodatasetse=<dataset>] 
  """
  Given args which can be either a dataset, a block, or an lfn, it returns sites
  """
  sites = []
  if sum(map(lambda x: 1 if '/' in x else 0, args)) > 3 : # "file"
    for filedata in rucio_client.list_replicas([{"scope": "cms", "name": args}]):
      sites = [ s for s in filedata['states'].keys() if 'AVAILABLE' in filedata['states'][s] ]
  else : # "block" or dataset"
    outfiles, outsites, sites_counts = get_dataset_files_replicas(
      args,
      allowlist_sites=sites_whitelist,
      blocklist_sites=sites_blacklist,
      regex_sites=sites_regex,
      mode="full",
      client=rucio_client,
      scope=scope
    )
    sorted_sites = dict(
      sorted(sites_counts.items(), key=lambda x: x[1], reverse=True)
    )
    # print(f"Loop over {len(outfiles)} outfiles:")
    # for f in outfiles:
    #   print(f)
    Nfiles = len(outfiles)
    for i, (site, stat) in enumerate(sorted_sites.items()):
      sites.append([site, f"{stat} / {Nfiles}", f"{stat*100/Nfiles:.1f}%"])
    return sites
  return sites

def do_list_parent_dids(args):
  """
  def list_parent_dids(args) from /cvmfs/cms.cern.ch/rucio/x86_64/rhel8/py3/current/bin/rucio
  Given args which can be either a block or an lfn with, it returns the parent dataset
  """
  ds = ''
  child = 'cms:'+args
  try:
    scope, name = extract_scope(child)
  except TypeError:
    scopes = rucio_client.list_scopes()
    scope, name = extract_scope(child, scopes)
  for dataset in rucio_client.list_parent_dids(scope=scope, name=name):
    ds = dataset['name']
  return ds


def do_get_lfn_dataset(args):
  block = do_list_parent_dids(args)
  return block.split('#')[0]

def get_dataset_sites_100_this_lfn ( sample_file = '/store/mc/test.root' ):
  dataset_name = do_get_lfn_dataset(sample_file)
  sites = do_list_sites_from_file_replicas(dataset_name)
  full_sites = []
  for site_p in sites :
    if site_p[2] == "100.0%":
      if "Tape" in site_p[0]: continue
      if site_p[0].startswith("T1_") or site_p[0].startswith("T2_"):
        site = site_p[0].replace("_Disk","")
        if not site in full_sites: full_sites.append(site)

  return full_sites

def get_dataset_sites_100 (dataset_name):
  # print(f"HERE1 = dataset_name : {dataset_name}")
  sites = do_list_sites_from_file_replicas(dataset_name)
  # print(sites)
  full_sites = []
  for site_p in sites :
    if site_p[2] == "100.0%":
      if "Tape" in site_p[0]: continue
      if site_p[0].startswith("T1_") or site_p[0].startswith("T2_") or site_p[0].startswith("T3_"):
        site = site_p[0].replace("_Disk","")
        if not site in full_sites: full_sites.append(site)
  # print(f"HERE2 = full_sites : ")
  # print(full_sites)
  return full_sites

def getStorageJson ( site ):
  import os.path
  from os import path
  import json

  storage_json='/cvmfs/cms.cern.ch/SITECONF/'+site+'/storage.json'
  if not path.exists(storage_json) : print ( 'no_storage_json' )
  with open(storage_json) as f: storage = f.read()
  try:
    storage = json.loads(str(storage))
  except:
    return {}
  return storage

def getFilepfnFromlfn (s,lfn):
  # Search pfn that has file:/
  import re
  storage = getStorageJson ( s )
  pfns = []
  for index in range(len(storage)):
    if storage[index]['rse'] is None : continue
    if not storage[index]['rse'] is None and 'Tape' in storage[index]['rse'] : continue
    if not storage[index]['rse'] is None and 'Buffer' in storage[index]['rse'] : continue
    for iproto in range(len(storage[index]['protocols'])):
      if 'rules' in storage[index]['protocols'][iproto].keys() :
        for irule in range(len(storage[index]['protocols'][iproto]['rules'])) :
          rule = storage[index]['protocols'][iproto]['rules'][irule]['lfn']
          pfn = storage[index]['protocols'][iproto]['rules'][irule]['pfn']
          match = re.match(rule, lfn)
          if match :
             for count, s in enumerate(match.groups()):
                  ar='$'+str(count+1)
                  pfn = pfn.replace(ar,s)
             if 'file:/' in pfn : pfns.append(pfn.replace('file:',''))
          else :
             pass
      elif 'prefix' in storage[index]['protocols'][iproto].keys() :
          pfn = storage[index]['protocols'][iproto]['prefix']+lfn
          if 'file:/' in pfn : pfns.append(pfn.replace('file:',''))
  return pfns

def get_filename_new_normalization(filename=None):
  import re
  import random
  try: import xml.etree.ElementTree as ET
  except ImportError: from elementtree import ElementTree as ET
  from datetime import datetime

  pfnsjsn = list ( set  (getFilepfnFromlfn('local',filename)))
  if len(pfnsjsn) > 0:
    if os.path.exists( pfnsjsn[0] ): return pfnsjsn[0] # 'file:/'+pfnsjsn[0]

  pick_sites=os.environ['CMS_DATASET_LOCATIONS'].split(",")

  SITE_LOCAL = os.path.realpath('/cvmfs/cms.cern.ch/SITECONF/local').split("/")[-1]

  filename_new = ''

  if SITE_LOCAL in pick_sites:
    SITE_STORAGE_JSON='/cvmfs/cms.cern.ch/SITECONF/local/storage.json'
    try:
      storage = json.load(open(SITE_STORAGE_JSON))
    except Exception as e:
      print(e) # for any exception to be catched
      print(type(e)) # to know the type of exception.
      storage = {}

    SITE_LOCAL_CONFIG='/cvmfs/cms.cern.ch/SITECONF/local/JobConfig/site-local-config.xml'
    pfn=''
    try:
      XML = ET.parse(SITE_LOCAL_CONFIG)
      event_data = XML.getroot().findall('.//data-access')
    except :
      event_data = []

    pfns = []
    for ed in event_data:
      catalog = ed.findall('.//catalog')
      for i in catalog:
        volume = [str( i.attrib['volume'] )]
        if 'Federation' in volume : continue
        protocols = [str( i.attrib['protocol'] )] # data-access
        for protocol in protocols:
          for index in range(len(storage)):
            if storage[index]['rse'] is None : continue
            if 'Tape' in storage[index]['rse'] : continue
            if 'Buffer' in storage[index]['rse'] : continue
            for iproto in range(len(storage[index]['protocols'])):
              if not protocol in storage[index]['protocols'][iproto]['protocol'] : continue
              if 'rules' in storage[index]['protocols'][iproto].keys() :
                for irule in range(len(storage[index]['protocols'][iproto]['rules'])):
                  rule = storage[index]['protocols'][iproto]['rules'][irule]['lfn']
                  pfn = storage[index]['protocols'][iproto]['rules'][irule]['pfn']
                  match = re.match(rule, filename)
                  if match :
                    for count, s in enumerate(match.groups()):
                      ar='$'+str(count+1)
                      pfn = pfn.replace(ar,s)
                      pfns.append(pfn)
                  else:
                    pass
              elif 'prefix' in storage[index]['protocols'][iproto].keys() :
                pfn = storage[index]['protocols'][iproto]['prefix']+filename
                pfns.append(pfn)
    print(f"pfns:")
    print(pfns)
    pfns_local = []
    for pfn in pfns:
      if str(pfn).startswith("/") or str(pfn).startswith("file"):
        pfns_local.append(pfn)
        break
    for pfn in pfns:
      if pfn in pfns_local: continue
      if not str(pfn) : continue
      if str(pfn).startswith("srm") : continue
      if str(pfn).startswith("dcap") : continue
      if str(pfn).startswith("gsiftp") : continue
      pfns_local.append(pfn)

    if len(pfns) > 0 :
      filename_new=pfns_local[0]
      if False:
         try:
            tree = uproot.open(pfns_local[0]) # ,ssl = sslctx)
            print("DEBUG bockjoo executor.py get_filename tree ", tree["Events"].num_entries)
            tree.close()
            filename_new=pfns_local[0]
            print("DEBUG bockjoo executor.py get_filename uproot.open fine")
         except Exception as e:
            pass

  if not filename_new:
    site_of_choice=random.choice(pick_sites)
    filename_new='root://cms-xrd-global.cern.ch:1094//store/test/xrootd/'+site_of_choice+filename
    if False:
      try :
        tree = uproot.open(filename_new)
        tree.close()
        print ("DEBUG bockjoo executor.py get_filename tree open complete")
      except :
        for soc in pick_sites :
          if soc in site_of_choice : continue
          filename_new='root://cms-xrd-global.cern.ch:1094//store/test/xrootd/'+soc+filename
          try :
            tree = uproot.open(filename_new)
            tree.close()
            break
          except :
            continue
  return filename_new

def sortSitesByDistance ( AVAIL_SITES = [], distmax = 9999.0 ):
  SITE_LAT_LONG_JSON="/cvmfs/cms.cern.ch/SITELATLONG.json"
  try:
    sitelatlongs = json.load(open(SITE_LAT_LONG_JSON))
  except Exception as e:
    sitelatlongs = []
  SITE_LOCAL = os.path.realpath('/cvmfs/cms.cern.ch/SITECONF/local').split("/")[-1]
  LAT_LOCAL = 0.0
  LONG_LOCAL = 0.0
  for sitell in sitelatlongs: #.items() :
      #site = str(sitelatlong.keys())
    for site,latlong in sitell.items():
      if not SITE_LOCAL == site: continue
      LAT_LOCAL = latlong['lat']
      LONG_LOCAL = latlong['long']
      break
  # print ( " LAT_LOCAL ",LAT_LOCAL, " LONG_LOCAL ",LONG_LOCAL)
  dists = {}
  for sitell in sitelatlongs: #.items() :
    for site,latlong in sitell.items():
      if not site in AVAIL_SITES: continue
      dist_site = math.sqrt ( ( LAT_LOCAL - latlong['lat'] )**2 + (LONG_LOCAL-latlong['long'])**2 )
      if dist_site > distmax : continue
      dists[site] = dist_site
      continue

  sorted_sites = sorted(dists.items(), key=lambda x:x[1])
  sites = []
  for sorted_site in sorted_sites:
    sites.append(sorted_site[0])

  # print(sites)
  return sites

# def getFilesetInNearSites (dataset = "TTbarPowheg_Dilepton", black_sites = [], limit = 0,  isMC=True, n_per_fileset = 0, useHTTPS = False ):
    
#    pfn = list(getFileset ( dataset =dataset, redirector='', limit=1 )[dataset]["files"].keys())[0]   
#    all_sites_with_100_percent_files = get_dataset_sites_100_this_lfn ( sample_file = pfn ) # pfn.split('')[1] )
#    # distmax = 30 # , distmax = 80 (ES_PIC) from Florida
#    # print ( "all_sites_with_100_percent_files ",all_sites_with_100_percent_files )
#    # # sites_within_distmax = sortSitesByDistance ( all_sites_with_100_percent_files, distmax = float(distmax) )
#    # # print ( "sites_within_distmax ",sites_within_distmax )
#    # # os.environ['CMS_DATASET_LOCATIONS'] = ",".join(str(site) for site in sites_within_distmax if not site in black_sites )
#    # # print ( "sites_within_distmax_excluding_black_sites",os.environ['CMS_DATASET_LOCATIONS'].split(","))

def GetListOfFilesFromDASGOClient(datasetName):
  out = subprocess.check_output("dasgoclient -json --query 'file dataset=%s'" %(datasetName), shell=True)
  list_jsonDict = json.loads(out) # Get list of dictionary. Each element of list is for a file.
  fileList = []
  for data in list_jsonDict:
    # print(data)
    file_name = data["file"][0]["name"]
    fileList.append(file_name)
  return fileList

def MakeSampleListWithRedirector(sampleName,datasetName):
  print(f"Making files for {sampleName} from {datasetName}")

  #
  # Excluded black
  #
  black_sites = []
  # black_sites = ["T2_US_PURDUE","T2_PL_Cyfronet","T1_IT_CNAF"]
  black_sites = [
    "T2_CH_CSCS",
    "T2_FR_IPHC",
    "T1_US_FNAL",
    "T1_RU_JINR"
  ]

  #
  # For this dataset, get sites with 100% availability of files
  #
  all_sites_with_100_percent_files = get_dataset_sites_100( dataset_name =  datasetName )
  print("all_sites_with_100_percent_files = ")
  print(all_sites_with_100_percent_files)
  #
  #
  #
  all_sites_with_100_percent_files = [x for x in all_sites_with_100_percent_files if x not in black_sites]
  print("all_sites_with_100_percent_files after blacklist check = ")
  print(all_sites_with_100_percent_files)

  if len(all_sites_with_100_percent_files) > 0:
    # print(all_sites_with_100_percent_files)
    #
    # Sort sites by proximity to this site (i.e on lxplus, it should be T2_CH_CERN)
    #
    sites_within_distmax = []
    distmax=60.
    sites_within_distmax = sortSitesByDistance ( all_sites_with_100_percent_files, distmax = float(distmax) )
    print("sites_within_distmax = ")
    print(sites_within_distmax)
    if len(sites_within_distmax) <= 1:
      print("Unable to find 'close-by' sites or found only just 1. Expanding....")
      distmax=120.
      sites_within_distmax = sortSitesByDistance ( all_sites_with_100_percent_files, distmax = float(distmax) )
      print("updated sites_within_distmax = ")
      print(sites_within_distmax)
    if len(sites_within_distmax) <= 1:
      print("Still unable to find 'close-by' sites. Expanding....")
      distmax=360
      sites_within_distmax = sortSitesByDistance ( all_sites_with_100_percent_files, distmax = float(distmax) )
      print("next updated sites_within_distmax = ")
      print(sites_within_distmax)

    # os.environ['CMS_DATASET_LOCATIONS'] = ",".join(str(site) for site in sites_within_distmax if not site in black_sites )
    # print ( "sites_within_distmax_excluding_black_sites",os.environ['CMS_DATASET_LOCATIONS'].split(","))

    dict_site_prefix = {}
    for site in sites_within_distmax:
      if site in black_sites:
        print(f"Skipping site in blacklist: {site}")
        continue
      storage = getStorageJson ( site )
      for index in range(len(storage)):
        # Skip all this
        if storage[index]['rse'] is None : continue
        if not storage[index]['rse'] is None and 'Tape' in storage[index]['rse'] : continue
        if not storage[index]['rse'] is None and 'Buffer' in storage[index]['rse'] : continue
        #
        for iproto in range(len(storage[index]['protocols'])):
          # print(storage[index]['protocols'][iproto])
          if storage[index]['protocols'][iproto]["protocol"] == "XRootD":
            if 'prefix' in storage[index]['protocols'][iproto].keys():
              dict_site_prefix[site] = storage[index]['protocols'][iproto]["prefix"]
            elif 'rules' in storage[index]['protocols'][iproto].keys() :
              for irule in range(len(storage[index]['protocols'][iproto]['rules'])):
                pfn = storage[index]['protocols'][iproto]['rules'][irule]['pfn']
                dict_site_prefix[site] = pfn.replace("$1","")

    print(dict_site_prefix)
    if len(dict_site_prefix) == 0:
      raise Exception('No sites in final list! Please check')

    # Final list of sites
    pick_sites_final = list(dict_site_prefix.keys())
    print(pick_sites_final)

    hasT2FIHIP  = "T2_FI_HIP" in pick_sites_final
    hasT2CHCERN = "T2_CH_CERN" in pick_sites_final
    hasT1DEKIT = "T1_DE_KIT" in pick_sites_final
    hasT2BEIIHE = "T2_BE_IIHE" in pick_sites_final
    hasT1FRCCIN2P3 = "T1_FR_CCIN2P3" in pick_sites_final
    hasT2FRGRIF = "T2_FR_GRIF" in pick_sites_final
    hasT2CHCSCS = "T2_CH_CSCS" in pick_sites_final
    hasT1ITCNAF = "T1_IT_CNAF" in pick_sites_final
    hasT2IPHC = "T2_FR_IPHC" in pick_sites_final
    hasT2DEDESY = "T2_DE_DESY" in pick_sites_final
    hasT3PSI = "T3_CH_PSI" in pick_sites_final
    hasT2Florida = "T2_US_Florida" in pick_sites_final
    hasT2Wisconsin = "T2_US_Wisconsin" in pick_sites_final
    hasT2KIPT = "T2_UA_KIPT" in pick_sites_final
    hasT2ESIFCA = "T2_ES_IFCA" in pick_sites_final

    fileLFNFromDAS = GetListOfFilesFromDASGOClient(datasetName)
    finalFileList = []
    for pfn in fileLFNFromDAS:
      if hasT2CHCERN:
        site_of_choice = "T2_CH_CERN"
      elif hasT2FIHIP:
        site_of_choice = "T2_FI_HIP"
      elif hasT1DEKIT:
        site_of_choice = "T1_DE_KIT"
      elif hasT1FRCCIN2P3:
        site_of_choice = "T1_FR_CCIN2P3"
      elif hasT2BEIIHE:
        site_of_choice = "T2_BE_IIHE"
      elif hasT2FRGRIF:
        site_of_choice = "T2_FR_GRIF"
      elif hasT3PSI:
        site_of_choice = "T3_CH_PSI"
      elif hasT2CHCSCS:
        site_of_choice = "T2_CH_CSCS"
      elif hasT2DEDESY:
        site_of_choice = "T2_DE_DESY"
      elif hasT1ITCNAF:
        site_of_choice = "T1_IT_CNAF"
      elif hasT2IPHC:
        site_of_choice = "T2_FR_IPHC"
      elif hasT2Florida:
        site_of_choice = "T2_US_Florida"
      elif hasT2Wisconsin:
        site_of_choice = "T2_US_Wisconsin"
      elif hasT2KIPT:
        site_of_choice = "T2_UA_KIPT"
      elif hasT2ESIFCA:
        site_of_choice = "T2_ES_IFCA"
      # Randomly pick a site
      else:
        site_of_choice = random.choice(pick_sites_final)

      # print(f"site_of_choice = {site_of_choice}")

      prefix = dict_site_prefix[site_of_choice]
      #
      thefile = f"{prefix}{pfn}"
      finalFileList.append(thefile)

    #
    #
    #
    fout = f"{sampleName}.txt"
    fo = open(fout, "w")
    print(f"Making {fout} with nfiles={len(finalFileList)}")
    for file_name in finalFileList:
      fo.write(file_name+'\n')
    fo.close()
    print("")
  else:
    #
    # Where there is no single site with 100%, lets just randomly pick
    # the sites for each file
    #
    outfiles, outsites, sites_counts = get_dataset_files_replicas(
      datasetName,
      allowlist_sites=sites_whitelist,
      blocklist_sites=sites_blacklist,
      regex_sites=sites_regex,
      mode="full",
      client=rucio_client,
      scope=scope
    )
    sorted_sites = dict(
      sorted(sites_counts.items(), key=lambda x: x[1], reverse=True)
    )
    # print(f"Loop over {len(outfiles)} outfiles:")
    # for f in outfiles:print(f)
    # for f in sorted_sites:print(f)
    # print(sorted_sites)
    # Nfiles = len(outfiles)
    finalFileList=[]
    for f in outfiles:
      # Get the first file
      finalFileList.append(f[0])
    #
    #
    #
    fout = f"{sampleName}.txt"
    fo = open(fout, "w")
    print(f"Making {fout} with nfiles={len(finalFileList)}")
    for file_name in finalFileList:
      fo.write(file_name+'\n')
    fo.close()
    print("")


def main():
  datasetDict = {
    "MC26Winter_Pi_E-0p2to10"  :"/Pi_Par-E-0p2to10_PGun/Run3Winter26Digi-JMEFEVTNoHLTNoPU_150X_mcRun3_2026_realistic_v5-v1/GEN-SIM-RAW-RECO",
    "MC26Winter_Pi_E-0p2to200" :"/Pi_Par-E-0p2to200_PGun/Run3Winter26Digi-JMEFEVTNoHLTNoPU_150X_mcRun3_2026_realistic_v5-v1/GEN-SIM-RAW-RECO",
  }
  for sampleName in datasetDict:
    datasetName=datasetDict[sampleName]
    MakeSampleListWithRedirector(sampleName,datasetName)

if __name__ == "__main__":
  main()



