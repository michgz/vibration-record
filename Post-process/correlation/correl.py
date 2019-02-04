# Run with Python 2.7
##


from trace import Trace
from trace import ReadIn

import datetime
import os
import sys
import zipfile
import shutil
import math
import getopt
import numpy



def ODSDate(dt):
 return dt.strftime("<table:table-cell table:style-name=\"ce3\" office:value-type=\"date\" office:date-value=\"%Y-%m-%dT%H:%M:%S\" calcext:value-type=\"date\"><text:p>%d/%m/%Y %H:%M:%S</text:p></table:table-cell>")


def bIncludeTraces():
 return True


def correl(infile1, infile2, output, diff = float('nan')):


    # The time delta by which ADXL is _before_ LSM6.
    #
    # This was determined manually. Next big challenge: do it automatically!!
    #

    runningPath = os.path.dirname(os.path.abspath(__file__))


    adxl = []
    ax1 = ReadIn([infile1])
    for ay in ax1:
     az = ay.Calc()
     adxl.append([ay.dt,az[1],az[2]])

    lsm6 = []
    ax2 = ReadIn([infile2])
    for ay in ax2:
     az = ay.Calc()
     lsm6.append([ay.dt,az[1],az[2]])


    if (math.isnan(diff)):


        # Here were determine the time delta automatically.
        #
        
        # First, ensure that the two traces at least overlap in time.
        
        if (min([a[0] for a in adxl]) < max([a[0] for a in lsm6])) and (min([a[0] for a in lsm6]) < max([a[0] for a in adxl])):
            pass    # we're ok
        else:
            sys.exit(6)   # now okay
            
        # Now choose the top 25%
        
        min_len = min([len(adxl), len(lsm6)])
        
        if (min_len <= 10):
            choose_len = min_len
        else:
            choose_len = min_len /4;
            
            
        def sortKey(e):
            return e[1]  # or e[2]
        
        sorted_1 = sorted(adxl, key=sortKey, reverse=True)[0:choose_len]
        sorted_2 = sorted(lsm6, key=sortKey, reverse=True)[0:choose_len]
        
        
        #The following are in seconds
        epsilon = 20
        step_big = 10  # best if it's about 1/2 epsilon
        step_small = 1   # usually 1


        vals = range(-4000,4000,step_big)
            
        # First pass
        v3 = []
        for v in vals:
            b3 = 0
            for b1 in sorted_1:
                for b2 in sorted_2:
                    if (abs(b1[0]+datetime.timedelta(seconds=v)-b2[0]).total_seconds() < epsilon):
                        b3 += 1
                        break
            v3.append(b3)
            
        max_at = vals[v3.index(max(v3))]
        
        # Second pass
        vals = range(max_at - 2*step_big, max_at + 2*step_big, step_small)
        v3 = []
        for v in vals:
            b3 = 0
            for b1 in sorted_1:
                for b2 in sorted_2:
                    if (abs(b1[0]+datetime.timedelta(seconds=v)-b2[0]).total_seconds() < epsilon):
                        b3 += 1
                        break
            v3.append(b3)
            
        max_val = max(v3)
        
        # Now take the average of all values with the same maximum value
        
        max_at = [i for i, j in enumerate(v3) if j == max_val]
        
        time_diff = datetime.timedelta( seconds=vals[sum(max_at)/len(max_at)] )
                    
    else:

        time_diff = datetime.timedelta( seconds=diff )


    print "Using time difference of " + str(time_diff.total_seconds()) + " seconds"





    # The furtherest two times can be and still count as the same
    #
    epsilon = datetime.timedelta(seconds=15)




    print len(adxl)
    print len(lsm6)


    adxl_idx = len(adxl)*[-1]
    lsm6_idx = len(lsm6)*[-1]



    a1_idx = 0


    ## Find matching indices
    ##
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
     


    ## Choose some representative points
    ##
    bx2 = []
    bx_idx = 0
    for a1 in adxl:
        bx2.append(a1[1])   # or [2]
        bx_idx += 1
    bx1 = numpy.argsort(numpy.array(bx2))   # sorted indices
    
    num_bx = min(len(bx1) / 2, 100)    # number of indices to use.
    
    bx1 = bx1[0:num_bx]







    if os.path.abspath(output):
        theName = output
    else:
        theName = os.path.join(runningPath, output)



    if True:

     # Now output to a file


     i3 = 0
     for i1 in range(0,len(adxl_idx)):
      if (adxl_idx[i1] >= 0):
       i3 += 1
     noRows = i3 - 1

     print "noRows = %d" % noRows

     with open(   runningPath + "/content_local_obj1.xml", "w") as dest:
      with open(    runningPath +  "/6_2.xml", "r") as source:
       while True:
        copy_buff = source.read(4096)
        if not copy_buff:
         break
        dest.write(copy_buff)


      dest.write(    "<office:body>"  )
      dest.write(    "<office:chart>"  )
      dest.write(    "<chart:chart svg:width=\"25.17cm\" svg:height=\"14.295cm\" xlink:href=\"..\" xlink:type=\"simple\" chart:class=\"chart:scatter\" chart:style-name=\"ch1\">"  )
      dest.write(    "<chart:plot-area chart:style-name=\"ch2\" table:cell-range-address=\"Data.C1:Data.C%d Data.G1:Data.G%d\" svg:x=\"0.503cm\" svg:y=\"0.285cm\" svg:width=\"24.164cm\" svg:height=\"13.725cm\">"   %  (noRows, noRows)      )

      dest.write(    "<chartooo:coordinate-region svg:x=\"1.31cm\" svg:y=\"0.484cm\" svg:width=\"23.077cm\" svg:height=\"12.879cm\"/><chart:axis chart:dimension=\"x\" chart:name=\"primary-x\" chart:style-name=\"ch3\"/>"    )

      dest.write(    "<chart:axis chart:dimension=\"y\" chart:name=\"primary-y\" chart:style-name=\"ch3\"><chart:grid chart:style-name=\"ch4\" chart:class=\"major\"/></chart:axis>"  )
      dest.write(    "<chart:series chart:style-name=\"ch5\" chart:values-cell-range-address=\"Data.G1:Data.G%d\" chart:class=\"chart:scatter\">"   %  noRows  )
      dest.write(    "<chart:domain table:cell-range-address=\"Data.C1:Data.C%d\"/><chart:data-point chart:repeated=\"%d\"/>"   % (noRows, noRows)  )
      dest.write(    "</chart:series>"  )
      
      dest.write(    "<chart:wall chart:style-name=\"ch6\"/><chart:floor chart:style-name=\"ch7\"/></chart:plot-area><table:table table:name=\"local-table\"><table:table-header-columns><table:table-column/>"  )
      dest.write(    "</table:table-header-columns>"  )


      dest.write(    "<table:table-columns><table:table-column table:number-columns-repeated=\"2\"/></table:table-columns>"  )

      dest.write(    "<table:table-header-rows>"  )
      dest.write(    "<table:table-row>"  )
      dest.write(    "<table:table-cell><text:p/></table:table-cell>"  )
      dest.write(    "<table:table-cell office:value-type=\"string\"><text:p>Column C</text:p></table:table-cell><table:table-cell office:value-type=\"string\"><text:p>Column G</text:p></table:table-cell>"  )
      dest.write(    "</table:table-row>"  )
      dest.write(    "</table:table-header-rows>"  )





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


      dest.write(    "<office:body>"  )
      dest.write(    "<office:chart>"  )
      dest.write(    "<chart:chart svg:width=\"25.17cm\" svg:height=\"14.295cm\" xlink:href=\"..\" xlink:type=\"simple\" chart:class=\"chart:scatter\" chart:style-name=\"ch1\">"  )
      dest.write(    "<chart:plot-area chart:style-name=\"ch2\" table:cell-range-address=\"Data.B1:Data.B%d Data.F1:Data.F%d\" svg:x=\"0.503cm\" svg:y=\"0.285cm\" svg:width=\"24.164cm\" svg:height=\"13.725cm\">"   %  (noRows, noRows)      )

      dest.write(    "<chartooo:coordinate-region svg:x=\"1.124cm\" svg:y=\"0.484cm\" svg:width=\"23.357cm\" svg:height=\"12.879cm\"/><chart:axis chart:dimension=\"x\" chart:name=\"primary-x\" chart:style-name=\"ch3\"/>"   )

      dest.write(    "<chart:axis chart:dimension=\"y\" chart:name=\"primary-y\" chart:style-name=\"ch3\"><chart:grid chart:style-name=\"ch4\" chart:class=\"major\"/></chart:axis>"  )
      dest.write(    "<chart:series chart:style-name=\"ch5\" chart:values-cell-range-address=\"Data.F1:Data.F%d\" chart:class=\"chart:scatter\">"   %  noRows  )
      dest.write(    "<chart:domain table:cell-range-address=\"Data.B1:Data.B%d\"/><chart:data-point chart:repeated=\"%d\"/>"   % (noRows, noRows)  )
      dest.write(    "</chart:series>"  )
      
      dest.write(    "<chart:wall chart:style-name=\"ch6\"/><chart:floor chart:style-name=\"ch7\"/></chart:plot-area><table:table table:name=\"local-table\"><table:table-header-columns><table:table-column/>"  )
      dest.write(    "</table:table-header-columns>"  )


      dest.write(    "<table:table-columns><table:table-column table:number-columns-repeated=\"2\"/></table:table-columns>"  )

      dest.write(    "<table:table-header-rows>"  )
      dest.write(    "<table:table-row>"  )
      dest.write(    "<table:table-cell><text:p/></table:table-cell>"  )
      dest.write(    "<table:table-cell office:value-type=\"string\"><text:p>Column B</text:p></table:table-cell><table:table-cell office:value-type=\"string\"><text:p>Column F</text:p></table:table-cell>"  )
      dest.write(    "</table:table-row>"  )
      dest.write(    "</table:table-header-rows>"  )






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

      dest.write(    "<table:table table:name=\"Raw\" table:style-name=\"ta1\">"    )
      dest.write(    "<table:table-column table:style-name=\"co2\" table:number-columns-repeated=\"7\" table:default-cell-style-name=\"Default\"/>"   )
      if bIncludeTraces():
            for bi in bx1:
                ## Find the corresponding point
                ci = adxl_idx[bi]
                dest.write(    "<table:table-row table:style-name=\"ro1\">"    )
                dest.write(       ODSDate(ax1[bi].dt)      )
                dest.write(   "<table:table-cell/><table:table-cell/><table:table-cell/>"   )
                dest.write(       ODSDate(ax2[ci].dt)      )
                dest.write(   "</table:table-row>"   )
                for ii in range(0, 500):
                    dest.write(    "<table:table-row table:style-name=\"ro1\">"    )
                    if (ii < len(ax1[bi].x)):
                        f3 = ax1[bi].x[ii]
                        dest.write(   "<table:table-cell office:value-type=\"float\" office:value=\"%0.3f\" calcext:value-type=\"float\"><text:p>%0.3f</text:p></table:table-cell>"   %   (f3, f3)    )
                        f3 = ax1[bi].y[ii]
                        dest.write(   "<table:table-cell office:value-type=\"float\" office:value=\"%0.3f\" calcext:value-type=\"float\"><text:p>%0.3f</text:p></table:table-cell>"   %   (f3, f3)    )
                        f3 = ax1[bi].z[ii]
                        dest.write(   "<table:table-cell office:value-type=\"float\" office:value=\"%0.3f\" calcext:value-type=\"float\"><text:p>%0.3f</text:p></table:table-cell>"   %   (f3, f3)    )
                    else:
                        dest.write(   "<table:table-cell/><table:table-cell/><table:table-cell/>"   )
                    dest.write(   "<table:table-cell/>"   )
                    if (ii < len(ax2[ci].x)):
                        f3 = ax2[ci].x[ii]
                        dest.write(   "<table:table-cell office:value-type=\"float\" office:value=\"%0.3f\" calcext:value-type=\"float\"><text:p>%0.3f</text:p></table:table-cell>"   %   (f3, f3)    )
                        f3 = ax2[ci].y[ii]
                        dest.write(   "<table:table-cell office:value-type=\"float\" office:value=\"%0.3f\" calcext:value-type=\"float\"><text:p>%0.3f</text:p></table:table-cell>"   %   (f3, f3)    )
                        f3 = ax2[ci].z[ii]
                        dest.write(   "<table:table-cell office:value-type=\"float\" office:value=\"%0.3f\" calcext:value-type=\"float\"><text:p>%0.3f</text:p></table:table-cell>"   %   (f3, f3)    )
                    else:
                        dest.write(   "<table:table-cell/><table:table-cell/><table:table-cell/>"   )
                    dest.write(   "</table:table-row>"   )
      full_len = len(bx1)*501
      dest.write(    "</table:table>"   )

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


      ## Now write out the third table sheet

      dest.write(    "<table:table table:name=\"Compare\" table:style-name=\"ta1\">"    )
      dest.write(    "<table:shapes>"   )
      
      dest.write(    "<draw:frame draw:z-index=\"0\" draw:style-name=\"gr1\" draw:text-style-name=\"P1\" svg:width=\"278.64mm\" svg:height=\"168.14mm\" svg:x=\"85.08mm\" svg:y=\"19.65mm\">"  )
      dest.write(    "<draw:object draw:notify-on-update-of-ranges=\"Compare.D3:Compare.D502 Compare.E3:Compare.E502 Compare.D3:Compare.D502 Compare.F3:Compare.F502 Compare.D3:Compare.D502 Compare.G3:Compare.G502\" xlink:href=\"./Object 3\" xlink:type=\"simple\" xlink:show=\"embed\" xlink:actuate=\"onLoad\"><loext:p/>"   )
      dest.write(    "</draw:object>"   )
      dest.write(    "<draw:image xlink:href=\"./ObjectReplacements/Object 3\" xlink:type=\"simple\" xlink:show=\"embed\" xlink:actuate=\"onLoad\"/>"   )
      dest.write(    "</draw:frame>"   )

      dest.write(    "<draw:frame draw:z-index=\"1\" draw:style-name=\"gr1\" draw:text-style-name=\"P1\" svg:width=\"306.74mm\" svg:height=\"172.65mm\" svg:x=\"372.25mm\" svg:y=\"17.8mm\">"   )
      dest.write(    "<draw:object draw:notify-on-update-of-ranges=\"Compare.I3:Compare.I502 Compare.J3:Compare.J502 Compare.I3:Compare.I502 Compare.K3:Compare.K502 Compare.I3:Compare.I502 Compare.L3:Compare.L502\" xlink:href=\"./Object 4\" xlink:type=\"simple\" xlink:show=\"embed\" xlink:actuate=\"onLoad\"><loext:p/>"    )
      dest.write(    "</draw:object>"   )
      dest.write(    "<draw:image xlink:href=\"./ObjectReplacements/Object 4\" xlink:type=\"simple\" xlink:show=\"embed\" xlink:actuate=\"onLoad\"/>"   )
      dest.write(    "</draw:frame>"   )

      dest.write(    "</table:shapes>"   )

      dest.write(    "<table:table-column table:style-name=\"co2\" table:number-columns-repeated=\"12\" table:default-cell-style-name=\"Default\"/>"   )

      dest.write(    "<table:table-row table:style-name=\"ro1\"/>"     )
      dest.write(    "<table:table-row table:style-name=\"ro1\"><table:table-cell/>"     )
      dest.write(    "<table:table-cell office:value-type=\"float\" office:value=\"1\" calcext:value-type=\"float\"><text:p>1</text:p></table:table-cell>"   )
      dest.write(    "<table:table-cell/><table:table-cell/>"    )
      dest.write(    "<table:table-cell table:formula=\"of:=INDEX([.$A$3:.$C$%d];[.$B$2];2)\" office:value-type=\"float\" office:value=\"502\" calcext:value-type=\"float\"><text:p>502</text:p></table:table-cell>"    %   (500)    )
      dest.write(    "</table:table-row>"     )
      
      
      for yi in range(0,500):
            dest.write(    "<table:table-row table:style-name=\"ro1\">"     )
            dest.write(    "<table:table-cell office:value-type=\"float\" office:value=\"%d\" calcext:value-type=\"float\"><text:p>%d</text:p></table:table-cell>"   %  (yi+1, yi+1)   )
            dest.write(    "<table:table-cell office:value-type=\"float\" office:value=\"%d\" calcext:value-type=\"float\"><text:p>%d</text:p></table:table-cell>"   %  (yi*501+1, yi*501+1)   )
            dest.write(    "<table:table-cell/>"   )
            dest.write(    "<table:table-cell office:value-type=\"float\" office:value=\"%d\" calcext:value-type=\"float\"><text:p>%d</text:p></table:table-cell>"   %  (yi+1, yi+1)   )
            dest.write(    "<table:table-cell table:formula=\"of:=INDEX([$Raw.$A$1:.$C$%d];[.$E$2]+[.$D%d];1)\" office:value-type=\"float\" office:value=\"0.0000\" calcext:value-type=\"float\"><text:p>0.0000</text:p></table:table-cell>"    %   (full_len+1, yi+3)    )
            dest.write(    "<table:table-cell table:formula=\"of:=INDEX([$Raw.$A$1:.$C$%d];[.$E$2]+[.$D%d];2)\" office:value-type=\"float\" office:value=\"0.0000\" calcext:value-type=\"float\"><text:p>0.0000</text:p></table:table-cell>"    %   (full_len+1, yi+3)    )
            dest.write(    "<table:table-cell table:formula=\"of:=INDEX([$Raw.$A$1:.$C$%d];[.$E$2]+[.$D%d];3)\" office:value-type=\"float\" office:value=\"0.0000\" calcext:value-type=\"float\"><text:p>0.0000</text:p></table:table-cell>"    %   (full_len+1, yi+3)    )
            dest.write(    "<table:table-cell/>"   )
            dest.write(    "<table:table-cell office:value-type=\"float\" office:value=\"%d\" calcext:value-type=\"float\"><text:p>%d</text:p></table:table-cell>"   %  (yi+1, yi+1)   )            
            dest.write(    "<table:table-cell table:formula=\"of:=INDEX([$Raw.$E$1:.$G$%d];[.$E$2]+[.$I%d];1)\" office:value-type=\"float\" office:value=\"0.0000\" calcext:value-type=\"float\"><text:p>0.0000</text:p></table:table-cell>"    %   (full_len+1, yi+3)    )
            dest.write(    "<table:table-cell table:formula=\"of:=INDEX([$Raw.$E$1:.$G$%d];[.$E$2]+[.$I%d];2)\" office:value-type=\"float\" office:value=\"0.0000\" calcext:value-type=\"float\"><text:p>0.0000</text:p></table:table-cell>"    %   (full_len+1, yi+3)    )
            dest.write(    "<table:table-cell table:formula=\"of:=INDEX([$Raw.$E$1:.$G$%d];[.$E$2]+[.$I%d];3)\" office:value-type=\"float\" office:value=\"0.0000\" calcext:value-type=\"float\"><text:p>0.0000</text:p></table:table-cell>"    %   (full_len+1, yi+3)    )
            dest.write(    "</table:table-row>"     )  
        
      #dest.write(    "<table:table-cell table:formula=\"of:=INDEX([$Raw.$A$1:.$C$%d];[.$B$2]+1;1)\" office:value-type=\"float\" office:value=\"0.0000\" calcext:value-type=\"float\"><text:p>0.0000</text:p></table:table-cell>"    %   (full_len+1)    )
      #dest.write(    "</table:table-row>"     )
      
      
      dest.write(    "</table:table>"   )



      ## Finish the document
      
      dest.write(       "</office:spreadsheet></office:body></office:document-content>" )


     ##### Now write to the ZIP archive  ####
     shutil.copy2(  runningPath + "/6_2.ODS", theName)  # 6_2 includes plots, 5_1 does not.
     with zipfile.ZipFile(theName, "a") as z:
      z.write(  runningPath +  "/content_local.xml", "content.xml", zipfile.ZIP_DEFLATED )
      z.write(  runningPath +  "/content_local_obj1.xml", "Object 1/content.xml", zipfile.ZIP_DEFLATED )
      z.write(  runningPath +  "/content_local_obj2.xml", "Object 2/content.xml", zipfile.ZIP_DEFLATED )
      z.close()

     os.remove(  runningPath +  "/content_local.xml"  )
     os.remove(  runningPath +  "/content_local_obj1.xml"  )
     os.remove(  runningPath +  "/content_local_obj2.xml"  )
     




######################################################################
##    START MAIN PROGRAM    ##
######################################################################


def usage():
    print "-h  help"
    print "-o  output"
    print "-d  time delta to use (if omitted, calculate automatically)"



def main():
    try:
        opts, args = getopt.gnu_getopt(sys.argv[1:], "ho:d:", ["help", "output=", "delta=", "diff="])
    except getopt.GetoptError as err:
        # print help information and exit:
        print str(err)  # will print something like "option -a not recognized"
        usage()
        sys.exit(2)
    Output = None
    Diff = float('nan')
    for o, a in opts:
        if o in ("-h", "--help"):
            usage()
            sys.exit()
        elif o in ("-o", "--output"):
            Output = a
        elif o in ("-d", "--delta", "--diff"):
            Diff = float(a)
        else:
            assert False, "unhandled option"
    if (Output == None):
        # Have no output. Fails.
        sys.exit(4)
        
        
    if (len(args) != 2):
        # Wrong number of args. Fails.
        sys.exit(4)
        thePath = args[0]


    if args[0].upper().endswith('.CSV'):
        input1 = os.path.abspath(args[0])
    else:
        sys.exit(3)
        
    if args[1].upper().endswith('.CSV'):
        input2 = os.path.abspath(args[1])
    else:
        sys.exit(3)


    if math.isnan(Diff):
        correl(input1, input2, output=Output)
    else:
        correl(input1, input2, output=Output, diff=Diff)
    





if __name__=="__main__":
    main()



