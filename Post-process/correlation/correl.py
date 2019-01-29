# Run with Python 2.7
##


import datetime
import os
import zipfile
import shutil



def ODSDate(dt):
 return dt.strftime("<table:table-cell table:style-name=\"ce3\" office:value-type=\"date\" office:date-value=\"%Y-%m-%dT%H:%M:%S\" calcext:value-type=\"date\"><text:p>%d/%m/%Y %H:%M:%S</text:p></table:table-cell>")




runningPath = os.path.dirname(os.path.abspath(__file__))
print "X"


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
time_diff = datetime.timedelta(seconds=-467)




# The furtherest two times can be and still count as the same
#
epsilon = datetime.timedelta(seconds=15)




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
 if (best_diff < epsilon) and (lsm6_idx[best_idx] == -1):
  adxl_idx[a1_idx] = best_idx
  lsm6_idx[best_idx] = a1_idx
 a1_idx += 1
 






theName = os.path.join(runningPath, '1.ods')


if True:

 # Now output to a file


 i3 = 0
 for i1 in range(0,len(adxl_idx)):
  if (adxl_idx[i1] >= 0):
   i3 += 1
 noRows = i3 - 1

 with open(   runningPath + "/content_local_obj1.xml", "w") as dest:
  with open(    runningPath +  "/6_2.xml", "r") as source:
   while True:
    copy_buff = source.read(4096)
    if not copy_buff:
     break
    dest.write(copy_buff)

    
  dest.write(    "<table:table-rows>"  )

  for i1 in range(0,len(adxl_idx)):

   if (adxl_idx[i1] >= 0):
    i3 += 1
    dest.write(       "<table:table-row>"    )
    dest.write(       "<table:table-cell office:value-type=\"string\"><text:p>%d</text:p></table:table-cell>"   % i3)
    f3 = adxl[i1][1]
    dest.write(   "<table:table-cell office:value-type=\"float\" office:value=\"%0.3f\"><text:p>%0.3f</text:p>"   % (f3, f3)   )
    if (i3 ==1 ):
     dest.write(   "<draw:g><svg:desc>Data.C1:Data.C%d</svg:desc></draw:g>"   % noRows   )
    dest.write(   "</table:table-cell>"   )
    f3 = lsm6[adxl_idx[i1]][1]
    dest.write(   "<table:table-cell office:value-type=\"float\" office:value=\"%0.3f\"><text:p>%0.3f</text:p>"   % (f3, f3)   )
    if (i3 ==1 ):
     dest.write(   "<draw:g><svg:desc>Data.G1:Data.G%d</svg:desc></draw:g>"   % noRows   )
    dest.write(    "</table:table-cell>"    )
    dest.write(       "</table:table-row>"     )

  dest.write(   "</table:table-rows></table:table></chart:chart></office:chart></office:body></office:document-content>"   )



 with open(   runningPath + "/content_local_obj2.xml", "w") as dest:
  with open(    runningPath +  "/7_2.xml", "r") as source:
   while True:
    copy_buff = source.read(4096)
    if not copy_buff:
     break
    dest.write(copy_buff)

    
  dest.write(    "<table:table-rows>"  )
  i4 = 0
  for i1 in range(0,len(adxl_idx)):

   if (adxl_idx[i1] >= 0):
    i4 += 1
    dest.write(       "<table:table-row>"    )
    dest.write(       "<table:table-cell office:value-type=\"string\"><text:p>%d</text:p></table:table-cell>"   % i4)
    f3 = adxl[i1][2]
    dest.write(   "<table:table-cell office:value-type=\"float\" office:value=\"%0.3f\"><text:p>%0.3f</text:p>"   % (f3, f3)   )
    if (i4 ==1 ):
     dest.write(   "<draw:g><svg:desc>Data.B1:Data.B%d</svg:desc></draw:g>"   % noRows   )
    dest.write(   "</table:table-cell>"   )
    f3 = lsm6[adxl_idx[i1]][2]
    dest.write(   "<table:table-cell office:value-type=\"float\" office:value=\"%0.3f\"><text:p>%0.3f</text:p>"   % (f3, f3)   )
    if (i4 ==1 ):
     dest.write(   "<draw:g><svg:desc>Data.F1:Data.F%d</svg:desc></draw:g>"   % noRows   )
    dest.write(    "</table:table-cell>"    )
    dest.write(       "</table:table-row>"     )

  dest.write(   "</table:table-rows></table:table></chart:chart></office:chart></office:body></office:document-content>"   )







 ######################################################
 ###  Write Header copperplate to the contents file  ##

 with open(    runningPath +  "/content_local.xml", "w") as dest:
  with open(    runningPath +  "/../template_content/3.xml", "r") as source:
   while True:
    copy_buff = source.read(4096)
    if not copy_buff:
     break
    dest.write(copy_buff)





  dest.write(       "<office:body>"    )
  dest.write(       "<office:spreadsheet>"    )
  dest.write(       "<table:calculation-settings table:automatic-find-labels=\"false\"/>"    )
  dest.write(       "<table:table table:name=\"Data\" table:style-name=\"ta1\">"    )
  dest.write(       "<table:shapes>"    )
  dest.write(       "<draw:frame draw:z-index=\"0\" draw:style-name=\"gr1\" draw:text-style-name=\"P1\" svg:width=\"251.69mm\" svg:height=\"142.94mm\" svg:x=\"223.06mm\" svg:y=\"7.17mm\">"    )
  dest.write(       "<draw:object draw:notify-on-update-of-ranges=\"Data.C1:Data.C%d Data.G1:Data.G%d\" xlink:href=\"./Object 1\" xlink:type=\"simple\" xlink:show=\"embed\" xlink:actuate=\"onLoad\">"  %  (noRows, noRows)  )
  dest.write(       "<loext:p/>"    )
  dest.write(       "</draw:object>"    )
  dest.write(       "<draw:image xlink:href=\"./ObjectReplacements/Object 1\" xlink:type=\"simple\" xlink:show=\"embed\" xlink:actuate=\"onLoad\"/>"    )
  dest.write(       "</draw:frame>"    )
  dest.write(       "<draw:frame draw:z-index=\"1\" draw:style-name=\"gr1\" draw:text-style-name=\"P1\" svg:width=\"251.69mm\" svg:height=\"142.94mm\" svg:x=\"409.83mm\" svg:y=\"79.95mm\">"    )
  dest.write(       "<draw:object draw:notify-on-update-of-ranges=\"Data.B1:Data.B%d Data.F1:Data.F%d\" xlink:href=\"./Object 2\" xlink:type=\"simple\" xlink:show=\"embed\" xlink:actuate=\"onLoad\">" % (noRows, noRows)   )
  dest.write(       "<loext:p/>"    )
  dest.write(       "</draw:object>"    )
  dest.write(       "<draw:image xlink:href=\"./ObjectReplacements/Object 2\" xlink:type=\"simple\" xlink:show=\"embed\" xlink:actuate=\"onLoad\"/>"    )
  dest.write(       "</draw:frame>"    )
  dest.write(       "</table:shapes>"    )
  dest.write(       "<table:table-column table:style-name=\"co1\" table:default-cell-style-name=\"ce3\"/>"    )
  dest.write(       "<table:table-column table:style-name=\"co2\" table:number-columns-repeated=\"3\" table:default-cell-style-name=\"Default\"/>"    )
  dest.write(       "<table:table-column table:style-name=\"co1\" table:default-cell-style-name=\"Default\"/>"    )
  dest.write(       "<table:table-column table:style-name=\"co2\" table:number-columns-repeated=\"2\" table:default-cell-style-name=\"Default\"/>"    )


 
  for i1 in range(0,len(adxl_idx)):

   if (adxl_idx[i1] >= 0):
    dest.write(       "<table:table-row table:style-name=\"ro1\">"    )
    dest.write(   ODSDate(adxl[i1][0])  )
    f3 = adxl[i1][1]
    dest.write(   "<table:table-cell office:value-type=\"float\" office:value=\"%0.3f\" calcext:value-type=\"float\"><text:p>%0.3f</text:p></table:table-cell>"    %  (f3, f3)   )
    f3 = adxl[i1][2]
    dest.write(   "<table:table-cell office:value-type=\"float\" office:value=\"%0.3f\" calcext:value-type=\"float\"><text:p>%0.3f</text:p></table:table-cell>"    %  (f3, f3)   )
    dest.write("<table:table-cell/>")
    dest.write(   ODSDate(lsm6[adxl_idx[i1]][0])  )
    f3 = lsm6[adxl_idx[i1]][1]
    dest.write(   "<table:table-cell office:value-type=\"float\" office:value=\"%0.3f\" calcext:value-type=\"float\"><text:p>%0.3f</text:p></table:table-cell>"    %  (f3, f3)   )
    f3 = lsm6[adxl_idx[i1]][2]
    dest.write(   "<table:table-cell office:value-type=\"float\" office:value=\"%0.3f\" calcext:value-type=\"float\"><text:p>%0.3f</text:p></table:table-cell>"    %  (f3, f3)   )
    dest.write("<table:table-cell/>")
    #dest.write(       "<table:table-cell office:value-type=\"string\" calcext:value-type=\"string\"><text:p>%s</text:p></table:table-cell>" % ("Hello!")   )
    dest.write(       "</table:table-row>"     )

  # Write empty row
  dest.write(       "<table:table-row table:style-name=\"ro1\"><table:table-cell/><table:table-cell/><table:table-cell/><table:table-cell/><table:table-cell/><table:table-cell/><table:table-cell/></table:table-row>"    )

  dest.write(       "<table:table-row table:style-name=\"ro1\">"   )
  dest.write(       "<table:table-cell office:value-type=\"string\" calcext:value-type=\"string\"><text:p>%s</text:p></table:table-cell>" % ("Unmatched:")   )
  dest.write(       "<table:table-cell/><table:table-cell/><table:table-cell/>"             )
  dest.write(       "<table:table-cell office:value-type=\"string\" calcext:value-type=\"string\"><text:p>%s</text:p></table:table-cell>" % ("Unmatched:")   )
  dest.write(       "<table:table-cell/><table:table-cell/>"             )
  dest.write(       "</table:table-row>"     )


  # Now write out the remaining values (that haven't been paired).

  i1 = 0
  i2 = 0
  while(True):
   while(i1 < len(adxl_idx)):
    if (adxl_idx[i1] != -1):
     i1 += 1
    else:
     break
   while(i2 < len(lsm6_idx)):
    if (lsm6_idx[i2] != -1):
     i2 += 1
    else:
     break
   if(i1 >= len(adxl_idx)) and (i2 >= len(lsm6_idx)):
    # Finished!
    break
   else:
    # Something still to do!
    dest.write(       "<table:table-row table:style-name=\"ro1\">"    )
    if(i1 < len(adxl_idx)):
     dest.write(   ODSDate(adxl[i1][0])  )
     f3 = adxl[i1][1]
     dest.write(   "<table:table-cell office:value-type=\"float\" office:value=\"%0.3f\" calcext:value-type=\"float\"><text:p>%0.3f</text:p></table:table-cell>"    %  (f3, f3)   )
     f3 = adxl[i1][2]
     dest.write(   "<table:table-cell office:value-type=\"float\" office:value=\"%0.3f\" calcext:value-type=\"float\"><text:p>%0.3f</text:p></table:table-cell>"    %  (f3, f3)   )
     i1 += 1
    else:
     dest.write(       "<table:table-cell/><table:table-cell/><table:table-cell/>"             )
    dest.write(        "<table:table-cell/>"             )
    if(i2 < len(lsm6_idx)):
     dest.write(   ODSDate(lsm6[i2][0])  )
     f3 = lsm6[i2][1]
     dest.write(   "<table:table-cell office:value-type=\"float\" office:value=\"%0.3f\" calcext:value-type=\"float\"><text:p>%0.3f</text:p></table:table-cell>"    %  (f3, f3)   )
     f3 = lsm6[i2][2]
     dest.write(   "<table:table-cell office:value-type=\"float\" office:value=\"%0.3f\" calcext:value-type=\"float\"><text:p>%0.3f</text:p></table:table-cell>"    %  (f3, f3)   )
     i2 += 1
    else:
     dest.write(       "<table:table-cell/><table:table-cell/><table:table-cell/>"             )
    dest.write(       "</table:table-row>"     )

  dest.write(       "</table:table>"     )
  dest.write(       "</office:spreadsheet></office:body></office:document-content>" )


 ##### Now write to the ZIP archive  ####
 shutil.copy2(  runningPath + "/5_1.ods", theName)
 with zipfile.ZipFile(theName, "a") as z:
  z.write(  runningPath +  "/content_local.xml", "content.xml", zipfile.ZIP_DEFLATED )
  z.write(  runningPath +  "/content_local_obj1.xml", "Object 1/content.xml", zipfile.ZIP_DEFLATED )
  z.write(  runningPath +  "/content_local_obj2.xml", "Object 2/content.xml", zipfile.ZIP_DEFLATED )
  z.close()

 os.remove(  runningPath +  "/content_local.xml"  )
 os.remove(  runningPath +  "/content_local_obj1.xml"  )
 os.remove(  runningPath +  "/content_local_obj2.xml"  )
 
