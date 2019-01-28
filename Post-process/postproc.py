# Run with Python 2.7
##


import zipfile
import shutil
import numpy
import os
import sys
import datetime


def bIncludeTraces():
 return False




######################################################################
##    START MAIN PROGRAM    ##
######################################################################



if len(sys.argv) < 2:
 sys.exit(0)


# Determine the output path and output file name based on the first argument.
thePath = sys.argv[1]


if not os.path.exists(thePath):
 sys.exit(0)

if os.path.isdir(thePath):
 # It's a directory. Use its parent for output

 thePath = os.path.abspath(thePath)

 if bIncludeTraces():
  theName = os.path.dirname(thePath) + "/" + os.path.basename(thePath) +       "_" + datetime.datetime.now().strftime("%y%m%d_%H%M%S") + ".ods"
 else:
  theName = os.path.dirname(thePath) + "/" + os.path.basename(thePath) + "_Short_" + datetime.datetime.now().strftime("%y%m%d_%H%M%S") + ".ods"

else:
 # It's a file. Use its parent as both the directory for input files
 # and for output files.

 thePath = os.path.abspath(thePath)

 if bIncludeTraces():
  theName = os.path.dirname(thePath) + "/" +            datetime.datetime.now().strftime("%y%m%d_%H%M%S") + ".ods"
 else:
  theName = os.path.dirname(thePath) + "/" + "Short_" + datetime.datetime.now().strftime("%y%m%d_%H%M%S") + ".ods"



# Go through all inputs adding on the found CSV files

inputFiles = []

for z in sys.argv[1:]:
 if os.path.isdir(z):
  inputFilesBare = sorted([y for y in os.listdir(z) if y.upper().endswith('.CSV')])
  for u in inputFilesBare:
   inputFiles.append(os.path.join(os.path.abspath(z), u))
 else:
  if z.upper().endswith('.CSV'):
   inputFiles.append(os.path.abspath(z))



argv_list = [os.path.abspath(__file__)]
argv_list.extend(inputFiles)
argv_list.append("--short")
argv_list.extend(["-o", theName])


execfile("postproc_spreadsheet.py", {"argv": argv_list})
