# Run with Python 2.7
##


import datetime
import os
import zipfile
import shutil



runningPath = os.path.dirname(os.path.abspath(__file__))



adxl = []
with open(os.path.join(runningPath,'ADXL.csv'),'r') as f1:
 for s in f1:
  t = s.strip().split('\t')
  a1 = datetime.datetime.strptime(t[0], '%d/%m/%Y %H:%M:%S')
  a2 = float(t[1])
  a3 = float(t[2])
  adxl.append([a1,a2,a3])

lsm6 = []
with open(os.path.join(runningPath,'LSM6.csv'),'r') as f1:
 for s in f1:
  t = s.strip().split('\t')
  a1 = datetime.datetime.strptime(t[0], '%d/%m/%Y %H:%M:%S')
  a2 = float(t[1])
  a3 = float(t[2])
  lsm6.append([a1,a2,a3])



# The time delta by which ADXL is _before_ LSM6.
#
# This was determined manually. Next big challenge: do it automatically!!
#
time_diff = datetime.timedelta(seconds=-389)




# The furtherest two times can be and still count as the same
#
epsilon = datetime.timedelta(seconds=30)




print len(adxl)
print len(lsm6)


adxl_idx = len(adxl)*[-1]
lsm6_idx = len(lsm6)*[-1]



a1_idx = 0


for a1 in adxl:
 targ_time = a1[0] + time_diff
 best_idx = 999999   # Just any value
 best_diff = datetime.timedelta.max
 a2_idx = 0
 for a2 in lsm6:
  diff = targ_time - a2[0]
  if (abs(diff) < best_diff):
   best_diff = abs(diff)
   best_idx = a2_idx
  a2_idx += 1
 if (best_diff < epsilon):
  adxl_idx[a1_idx] = best_idx
  lsm6_idx[best_idx] = a1_idx
 a1_idx += 1
 


print adxl_idx



theName = os.path.join(runningPath, '1.ods')


if True:

 # Now output to a file


 ######################################################
 ###  Write Header copperplate to the contents file  ##

 with open(    runningPath +  "/content_local.xml", "w") as dest:
  with open(    runningPath +  "/../template_content/1", "r") as source:
   while True:
    copy_buff = source.read(4096)
    if not copy_buff:
     break
    dest.write(copy_buff)



  dest.write(       "<office:body><office:spreadsheet><table:calculation-settings table:automatic-find-labels=\"false\"/>"     )
  dest.write(       "<table:table table:name=\"Data\" table:style-name=\"ta\">"      )
  dest.write(       "<table:table-row table:style-name=\"ro1\">"    )
  dest.write(       "<table:table-cell office:value-type=\"string\" calcext:value-type=\"string\"><text:p>%s</text:p></table:table-cell>" % ("Hello!")   )
  dest.write(       "</table:table-row>"     )
  dest.write(       "</table:table>"     )
  dest.write(       "</office:spreadsheet></office:body></office:document-content>" )


 ##### Now write to the ZIP archive  ####
 shutil.copy2(  runningPath + "/../template_content/3_1.ods", theName)
 with zipfile.ZipFile(theName, "a") as z:
  z.write(  runningPath +  "/content_local.xml", "content.xml", zipfile.ZIP_DEFLATED )
  z.close()

 os.remove(  runningPath +  "/content_local.xml")
