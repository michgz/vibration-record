# Run with Python 2.7
##


import zipfile
import shutil
import numpy
import os
import sys
import datetime


#### Set this variable to False if only a tabulated summary output is required #####
includeTraces = False



def bIncludeTraces():
 return includeTraces

def bAddHeartbeatsTable():
 return True

## Calculate square of a number
def sq(x):
 return 1.*x*x


class Trace:
 x = []
 y = []
 z = []
 data_pos = 0
 device_type = ""

 def Calc(self):
  if self.data_pos < 10:
   return [ self.data_pos, 0.0, 0.0 ]
  else:
   num = self.data_pos
   sum = [0., 0., 0.]
   for j in range(num):
    sum[0] += self.x[j]
    sum[1] += self.y[j]
    sum[2] += self.z[j]
   avg = [sum[0]/num, sum[1]/num, sum[2]/num]
   sumsq = 0.
   max_sq = 0.
   max_sq_pos = -1
   for j in range(num):
    pt_sumsq = 0.
    pt_sumsq += sq(self.x[j] - avg[0])
    pt_sumsq += sq(self.y[j] - avg[1])
    pt_sumsq += sq(self.z[j] - avg[2])
    if (pt_sumsq > max_sq):
     max_sq = pt_sumsq
     max_sq_pos = j
    sumsq += pt_sumsq
   # In case of ADXL, scale RMS measurements to be equivalent to LSM6
   #  -- now not needed; just use 1
   scaler = 1.
   #if (self.device_type == "ADXL"):
   # scaler = 256000./16384.
   return [ num, numpy.sqrt((1.0*(sumsq))/num) / scaler, numpy.sqrt(1.0*max_sq) / scaler ]

 def Clear(self):
  self.x = []
  self.y = []
  self.z = []
  self.data_pos = 0

 def Add(self, vector):
  if len(vector) >= 3:
   self.x.append(vector[0])
   self.y.append(vector[1])
   self.z.append(vector[2])
   self.data_pos += 1

 def GetLength(self):
  return self.data_pos


def StartNextTrace(file, start_row, next_dt):
 file.write(   "<table:table-cell/>"   )
 if start_row > 0:
  file.write(  "<table:table-cell office:value-type=\"float\" office:value=\"%d\" calcext:value-type=\"float\"><text:p>%d</text:p></table:table-cell>" % (start_row, start_row)   )
 else:
  file.write(  "<table:table-cell/>"    )
 file.write(   "<table:table-cell table:style-name=\"ce3\" office:value-type=\"date\" office:date-value=\"%04d-%02d-%02dT%02d:%02d:%02d\" calcext:value-type=\"date\">"    % (next_dt.year, next_dt.month, next_dt.day, next_dt.hour, next_dt.minute, next_dt.second )  )
 file.write(    "<text:p>%02d/%02d/%04d %02d:%02d:%02d</text:p>"    % (next_dt.day, next_dt.month, next_dt.year, next_dt.hour, next_dt.minute, next_dt.second )  )
 file.write(   "</table:table-cell>"   )
 file.write(   "<table:table-cell/>"    )


def MidOfTrace(file, device_type):
 if (device_type == "ADXL"):
  file.write(   "<table:table-cell office:value-type=\"string\" calcext:value-type=\"string\"><text:p>ADXL</text:p></table:table-cell>"   )
  this_freq = 125
 else:
  file.write(   "<table:table-cell office:value-type=\"string\" calcext:value-type=\"string\"><text:p>LSM6</text:p></table:table-cell>"   )
  this_freq = 104
 file.write(   "<table:table-cell office:value-type=\"float\" office:value=\"80\" calcext:value-type=\"float\"><text:p>80</text:p></table:table-cell>"   )
 file.write(   "<table:table-cell office:value-type=\"float\" office:value=\"%d\" calcext:value-type=\"float\"><text:p>%d</text:p></table:table-cell>" % (this_freq, this_freq)  )

def FlushPreviousTrace(file, prev_trace):
 # 12 cells
 cc = prev_trace.Calc()
 if (len(cc) >= 1):
  file.write(   "<table:table-cell office:value-type=\"float\" office:value=\"%d\" calcext:value-type=\"float\"><text:p>%d</text:p></table:table-cell>"    %  ( cc[0], cc[0] )    )
 else:
  file.write(   "<table:table-cell office:value-type=\"float\" office:value=\"0\" calcext:value-type=\"float\"><text:p>0</text:p></table:table-cell>"   )
 if (len(cc) >= 2):
  file.write(   "<table:table-cell office:value-type=\"float\" office:value=\"%0.3f\" calcext:value-type=\"float\"><text:p>%0.3f</text:p></table:table-cell>"    %  ( cc[1], cc[1] )    )
 else:
  file.write(      "<table:table-cell/>"           )        # no data -- leave as empty cell
 if (len(cc) >= 3):
  file.write(   "<table:table-cell office:value-type=\"float\" office:value=\"%0.3f\" calcext:value-type=\"float\"><text:p>%0.3f</text:p></table:table-cell>"    %  ( cc[2], cc[2] )    )
 else:
  file.write(   "<table:table-cell/>"           )        # no data -- leave as empty cell
 file.write(   "<table:table-cell/><table:table-cell/>\r\n"    )   # Note the EOL! It will be needed when we read this file back.


def WriteHeartbeatLine(file, dt, str1_line, str2_line, rr):
 file.write(   "<table:table-cell/>"   )
 file.write(   "<table:table-cell office:value-type=\"float\" office:value=\"%d\" calcext:value-type=\"float\"><text:p>%d</text:p></table:table-cell>"   %   (rr, rr)   )
 file.write(   "<table:table-cell table:style-name=\"ce3\" office:value-type=\"date\" office:date-value=\"%04d-%02d-%02dT%02d:%02d:%02d\" calcext:value-type=\"date\">"    % (dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second )  )
 file.write(    "<text:p>%02d/%02d/%04d %02d:%02d:%02d</text:p>"    % (dt.day, dt.month, dt.year, dt.hour, dt.minute, dt.second )  )
 file.write(   "</table:table-cell>"   )
 file.write(   "<table:table-cell office:value-type=\"string\" calcext:value-type=\"string\"><text:p>%s</text:p></table:table-cell>"    % str1_line   )
 #
 # Search for "S="
 #
 nn = str2_line.find( "S="  )
 if (nn >= 0):
  file.write(    ( "<table:table-cell office:value-type=\"string\" calcext:value-type=\"string\"><text:p>%s</text:p></table:table-cell>"   %  ( str2_line[nn+2:].split()[0] )  )    )
 else:
  file.write(   "<table:table-cell/>"   )   # Not found any data -- empty cell 
 #
 # Search for "Tacc"
 #
 nn = str2_line.lower().find( "tacc="  )
 if (nn >= 0):
  try:
   ff = float( str2_line[nn+5:].split()[0] )
  except ValueError:
   nn = -1
 if (nn >= 0):
  file.write(    ( "<table:table-cell office:value-type=\"float\" office:value=\"%f\" calcext:value-type=\"float\"><text:p>%f</text:p></table:table-cell>"   %  (ff, ff)  )    )
 else:
  file.write(   "<table:table-cell/>"   )   # Not found any data -- empty cell
 #
 # Search for "Tint"
 #
 nn = str2_line.lower().find( "tint="  )
 if (nn >= 0):
  try:
   ff = float( str2_line[nn+5:].split()[0] )
  except ValueError:
   nn = -1
 if (nn >= 0):
  file.write(    ( "<table:table-cell office:value-type=\"float\" office:value=\"%f\" calcext:value-type=\"float\"><text:p>%f</text:p></table:table-cell>"   %  (ff, ff)  )    )
 else:
  file.write(   "<table:table-cell/>"   )   # Not found any data -- empty cell
 #
 # Search for "Vbat"
 #
 nn = str2_line.lower().find( "vbat="  )
 if (nn >= 0):
  try:
   ff = float( str2_line[nn+5:].split()[0] )
  except ValueError:
   nn = -1
 if (nn >= 0):
  file.write(    ( "<table:table-cell office:value-type=\"float\" office:value=\"%f\" calcext:value-type=\"float\"><text:p>%f</text:p></table:table-cell>"   %  (ff, ff)  )    )
 else:
  file.write(   "<table:table-cell/>"   )   # Not found any data -- empty cell
 #
 # Search for "VCCIO"
 #
 nn = str2_line.lower().find( "vccio="  )
 if (nn >= 0):
  try:
   ff = float( str2_line[nn+6:].split()[0] )
  except ValueError:
   nn = -1
 if (nn >= 0):
  file.write(    ( "<table:table-cell office:value-type=\"float\" office:value=\"%f\" calcext:value-type=\"float\"><text:p>%f</text:p></table:table-cell>"   %  (ff, ff)  )    )
 else:
  file.write(   "<table:table-cell/>"   )   # Not found any data -- empty cell
 #
 # Search for "VCCCORE"
 #
 nn = str2_line.lower().find( "vcccore="  )
 if (nn >= 0):
  try:
   ff = float( str2_line[nn+8:].split()[0] )
  except ValueError:
   nn = -1
 if (nn >= 0):
  file.write(    ( "<table:table-cell office:value-type=\"float\" office:value=\"%f\" calcext:value-type=\"float\"><text:p>%f</text:p></table:table-cell>"   %  (ff, ff)  )    )
 else:
  file.write(   "<table:table-cell/>"   )   # Not found any data -- empty cell
 file.write(    "\r\n"   )   # Important! 
 


def IsLikelyDateTime(str):
 if len(str) < 16:
  return False
 if str[2] == '/' and str[5] == '/':
  return True
 return False






######################################################################
##    START MAIN PROGRAM    ##
######################################################################


runningPath = os.path.dirname(os.path.realpath(__file__))   # the directory of this .PY file

# Open log file
with open( runningPath + "/log.txt", "a") as f9:

 f9.write( datetime.datetime.now().strftime("Date: %d/%m/%Y %H:%M:%S\r") )
 f9.write( "Output directory: %s\r" % runningPath)

 if len(sys.argv) != 2:
  f9.write( "ERROR: %d input arguments, expected 2. Terminating\r" % len(sys.argv) )
  sys.exit(0)

 thePath = sys.argv[1]

 if not os.path.isabs(thePath):
  # Turn a relative path into an absolute one
  thePath = os.getcwd() + "/" + sys.argv[1]

 f9.write( "Input path: %s\r" % thePath )

 if not os.path.exists(thePath):
  f9.write( "ERROR: Input path doesn't exist. Terminating\r")
  sys.exit(0)

 if not os.path.isdir(thePath):
  f9.write( "ERROR: Input path is not a directory. Terminating\r")
  sys.exit(0)

 if not os.path.basename(thePath):
  thePath = os.path.dirname(thePath)  # remove final "/" if needed

 if bIncludeTraces():
  theName = os.path.dirname(thePath) + "/" + os.path.basename(thePath) + "_" + datetime.datetime.now().strftime("%y%m%d_%H%M%S") + ".ods"
 else:
  theName = os.path.dirname(thePath) + "/" + os.path.basename(thePath) + "_Short_" + datetime.datetime.now().strftime("%y%m%d_%H%M%S") + ".ods"

 f9.write( "Outputting to file: %s\r" % theName )

 # If we get here, we have a reasonable directory to process



 try:
  os.remove(  runningPath + "/content_local.xml")
 except:
  pass


 try:
  os.remove(   runningPath +  "/template_content/7")
 except:
  pass








 #######################################################
 ###  Read excluded traces from a specifically-named  ##
 ###  file, if it exists.                             ##

 exclude = []
 try:
  with open(thePath + '/Exclude.txt', 'r') as f3:
   for line in f3:
    # recognises ranges like "96-98"
    if ('-' in line):
     d = line.split('-')
     if (len(d) >= 2):
      exclude += range(int(d[0]), int(d[1])+1)
    else:
     try:
      exclude.append(int(line))
     except ValueError:
      d = [0,0]
   f9.write( "Found \"Exclude.txt\", excluding %d traces.\r" % len(exclude)  )
 except IOError:
  exclude = []




 ######################################################
 ###  Write Header copperplate to the contents file  ##

 with open(    runningPath +  "/content_local.xml", "w") as dest:
  with open(    runningPath +  "/template_content/1", "r") as source:
   while True:
    copy_buff = source.read(4096)
    if not copy_buff:
     break
    dest.write(copy_buff)


  f7 = open(runningPath +  "/7", "w")
  f8 = open(runningPath +  "/8", "w")
  
  
  ################################################
  ###  Process the inputs                       ##

  dest.write(       "<office:body><office:spreadsheet><table:calculation-settings table:automatic-find-labels=\"false\"/>"     )
  dest.write(       "<table:table table:name=\"Data\" table:style-name=\"ta\">"      )


  maxLength = 0
  is_probably_adxl_device = False

  current_trace = Trace()
  current_trace_line_number = 0
  current_trace_dt = datetime.datetime(2017, 1, 1, 0, 0)
  current_header_line = ""
  a = 0
  the_index = 0
  the_row = 0
  line_number = 1

  def bIsLineNumberOkay():
   # The following is a hard limit on the number of line in a sheet.
   return (line_number < 1048576)

  excluded = False
  for x in sorted([y for y in os.listdir(thePath) if y.endswith('.CSV')]):
   f9.write( "Processing input file: %s\r" % x )
   with open(thePath + '/' + x, 'r') as f1:
    for line in f1:
     if bIncludeTraces() and bIsLineNumberOkay():
      dest.write(      "<table:table-row table:style-name=\"ro1\">"    )
     if IsLikelyDateTime(line):
      d = line.split(',')
      if (len(d) >= 2):
       try:
        dt = datetime.datetime.strptime('%s,%s' % (d[0],d[1]), '%d/%m/%Y,%H:%M:%S')
       except ValueError:
        dt = datetime.datetime(2017, 1, 1, 0, 0)

       delta_dt = dt - current_trace_dt;
       if (delta_dt.total_seconds() >= 7) or (current_trace.data_pos < 250):
        # Each trace is 500 points long. At 104Hz or 125Hz that's about 5 seconds.
        # If the condition above is true, we should start a new trace, otherwise it
        # can be a continuation of the old one.
        #
        #
        # Check if there is a previous trace to flush
        if (current_trace.data_pos >= 250):
         StartNextTrace(f7, current_trace_line_number, current_trace_dt)
         MidOfTrace(f7, current_trace.device_type )
         FlushPreviousTrace(f7, current_trace)
         if (current_trace.data_pos > maxLength):
          maxLength = current_trace.data_pos
         the_index += 1

        # Start the new trace
        current_trace.Clear()
        if bIsLineNumberOkay():
         current_trace_line_number = line_number
        else:
         current_trace_line_number = -1
        current_trace_dt = dt
        current_trace.device_type = ""
        the_index += 1
        excluded = (the_index in exclude)

        # Now write to the Data sheet
        if bIncludeTraces() and bIsLineNumberOkay():
         dest.write(  (   "<table:table-cell table:style-name=\"ce1\" office:value-type=\"date\" office:date-value=\"%04d-%02d-%02d\" calcext:value-type=\"date\"><text:p>%02d/%02d/%04d</text:p></table:table-cell>"  % (dt.year, dt.month, dt.day, dt.day, dt.month, dt.year)   )     )
         dest.write(  (   "<table:table-cell table:style-name=\"ce2\" office:value-type=\"time\" office:time-value=\"PT%02dH%02dM%02dS\" calcext:value-type=\"time\"><text:p>%02d:%02d:%02d</text:p></table:table-cell>"   %  ( dt.hour, dt.minute, dt.second, dt.hour, dt.minute, dt.second )    )    )
         dest.write(      "<table:table-cell/>"       )

        the_row += 1
       a = 1
     else:
      if (a==1):
       # just copy across the string value
       a += 1
       if (line.find("ADXL") >= 0):
        current_trace.device_type  = "ADXL"
        is_probably_adxl_device = True # any ADXL we decide it is all ADXL.
       else:  # assume it's LSM!
        current_trace.device_type  = "LSM6"
       if bIncludeTraces() and bIsLineNumberOkay():
        dest.write(      "<table:table-cell office:value-type=\"string\" calcext:value-type=\"string\"><text:p>%s</text:p></table:table-cell>" % (line.split(',')[0])   )
        dest.write(      "<table:table-cell table:number-columns-repeated=\"2\"/>"     )
       current_header_line = line.strip()  # Only used if next line is "HEARTBEAT"
       the_row += 1
      elif (a==2) and (line.find("HEARTBEAT") >= 0):
       # It's a heartbeat line. Just copy the string value
       if bIncludeTraces() and bIsLineNumberOkay():
        dest.write(      "<table:table-cell office:value-type=\"string\" calcext:value-type=\"string\"><text:p>%s</text:p></table:table-cell>" % line   )
        dest.write(      "<table:table-cell table:number-columns-repeated=\"2\"/>"     )
       WriteHeartbeatLine(f8, current_trace_dt, line.strip(), current_header_line, the_row)
       the_row += 1
       a += 1
      else:
       d = line.split(',')
       d_num = [0, 0, 0]
       if (len(d) >= 1):
        try:
         d_num[0] = float(d[0].strip())
         if bIncludeTraces() and bIsLineNumberOkay():
          dest.write(    ( "<table:table-cell office:value-type=\"float\" office:value=\"%d\" calcext:value-type=\"float\"><text:p>%d</text:p></table:table-cell>"   %  (d_num[0], d_num[0])  )    )
        except ValueError:
         if bIncludeTraces() and bIsLineNumberOkay():
          dest.write(      "<table:table-cell/>"           )        # cannot convert -- leave as empty cell
       if (len(d) >= 2):
        try:
         d_num[1] = float(d[1].strip())
         if bIncludeTraces() and bIsLineNumberOkay():
          dest.write(    ( "<table:table-cell office:value-type=\"float\" office:value=\"%d\" calcext:value-type=\"float\"><text:p>%d</text:p></table:table-cell>"   %  (d_num[1], d_num[1])  )    )
        except ValueError:
         if bIncludeTraces() and bIsLineNumberOkay():
          dest.write(      "<table:table-cell/>"           )        # cannot convert -- leave as empty cell
       if (len(d) >= 3):
        try:
         d_num[2] = float(d[2].strip())
         if bIncludeTraces() and bIsLineNumberOkay():
          dest.write(    ( "<table:table-cell office:value-type=\"float\" office:value=\"%d\" calcext:value-type=\"float\"><text:p>%d</text:p></table:table-cell>"   %  (d_num[2], d_num[2])  )    )
        except ValueError:
         if bIncludeTraces() and bIsLineNumberOkay():
          dest.write(      "<table:table-cell/>"           )        # cannot convert -- leave as empty cell
        current_trace.Add(d_num)
       the_row += 1
       a += 1
     if bIncludeTraces() and bIsLineNumberOkay():
      dest.write(      "</table:table-row>"     )
     if bIsLineNumberOkay():
      line_number += 1

  if (current_trace.data_pos >= 250):
   # Flush the final trace
   StartNextTrace(f7, current_trace_line_number, current_trace_dt)
   MidOfTrace(f7, current_trace.device_type )
   FlushPreviousTrace(f7, current_trace)
   if (current_trace.data_pos > maxLength):
    maxLength = current_trace.data_pos
   the_index += 1


  f8.close()
  f7.close()


  ######################################################
  ###  End the Data sheet                  ##

  dest.write(      "</table:table>"    )


  f9.write( "Processed a total of %d traces from %d lines of CSV.\r" % (the_index, the_row)  )
  f9.write( "Maximum length detected: %d\r" % maxLength )



  ######################################################
  ### Write in information related to the graphics    ##


  dest.write("<table:table table:name=\"List\" table:style-name=\"ta1\">")


  if bIncludeTraces():
   dest.write(    "<table:shapes><draw:frame draw:z-index=\"0\" draw:style-name=\"gr1\" draw:text-style-name=\"P1\" svg:width=\"329.58mm\" svg:height=\"182.48mm\" svg:x=\"258.82mm\" svg:y=\"36.39mm\">"     )
   dest.write(    "<loext:p draw:notify-on-update-of-ranges=\""    );

   dest.write(    "List.N7:List.N%d " % (maxLength + 6))
   dest.write(    "List.O6:List.O6 "   )
   dest.write(    "List.O7:List.O%d " % (maxLength + 6))
   dest.write(    "List.N7:List.N%d " % (maxLength + 6))
   dest.write(    "List.P6:List.P6 "   )
   dest.write(    "List.P7:List.P%d " % (maxLength + 6))
   dest.write(    "List.N7:List.N%d " % (maxLength + 6))
   dest.write(    "List.Q6:List.Q6 "  )
   dest.write(    "List.Q7:List.Q%d\"/>" % (maxLength + 6))

   dest.write(    "<draw:object xlink:href=\"./Object 1\" xlink:type=\"simple\" xlink:show=\"embed\" xlink:actuate=\"onLoad\"/>"     )
   dest.write(    "<draw:image xlink:href=\"./ObjectReplacements/Object 1\" xlink:type=\"simple\" xlink:show=\"embed\" xlink:actuate=\"onLoad\"/>"    )
   dest.write(    "</draw:frame></table:shapes>"    )



  ######################################################
  ### Now write the tables                            ##


  dest.write(   "<table:table-column table:style-name=\"co1\" table:number-columns-repeated=\"2\" table:default-cell-style-name=\"Default\"/><table:table-column table:style-name=\"co2\" table:default-cell-style-name=\"ce3\"/><table:table-column table:style-name=\"co1\" table:number-columns-repeated=\"10\" table:default-cell-style-name=\"Default\"/><table:table-column table:style-name=\"co2\" table:default-cell-style-name=\"Default\"/><table:table-column table:style-name=\"co1\" table:number-columns-repeated=\"3\" table:default-cell-style-name=\"Default\"/>"    )
  dest.write(   "<table:table-row table:style-name=\"ro1\">"   )
  dest.write(   "<table:table-cell/>"   )
  dest.write(   "<table:table-cell office:value-type=\"string\" calcext:value-type=\"string\"><text:p>Row Number</text:p></table:table-cell>"   )
  dest.write(   "<table:table-cell office:value-type=\"string\" calcext:value-type=\"string\"><text:p>Date/Time</text:p></table:table-cell>"   )
  dest.write(   "<table:table-cell/>"   )
  dest.write(   "<table:table-cell office:value-type=\"string\" calcext:value-type=\"string\"><text:p>Accelerometer device</text:p></table:table-cell>"   )
  dest.write(   "<table:table-cell office:value-type=\"string\" calcext:value-type=\"string\"><text:p>Trigger (g/16384)</text:p></table:table-cell>"   )
  dest.write(   "<table:table-cell office:value-type=\"string\" calcext:value-type=\"string\"><text:p>Sample frequency (Hz)</text:p></table:table-cell>"   )
  dest.write(   "<table:table-cell office:value-type=\"string\" calcext:value-type=\"string\"><text:p>Sample length</text:p></table:table-cell>"   )
  dest.write(   "<table:table-cell office:value-type=\"string\" calcext:value-type=\"string\"><text:p>Max acceleration (g/16384)</text:p></table:table-cell>"   )
  dest.write(   "<table:table-cell office:value-type=\"string\" calcext:value-type=\"string\"><text:p>Root-total-square acceleration (g/16384)</text:p></table:table-cell>"   )
  dest.write(   "<table:table-cell table:number-columns-repeated=\"3\"/><table:table-cell table:style-name=\"Default\" table:number-columns-repeated=\"4\"/>"   )
  dest.write(   "</table:table-row>"    )


  constFrequencyVal = 104.



  def WriteIndexingRow(file_loc, i_loc, full_len_loc, list_len_loc):
   # Take a guess at time & amplitude scalers:
   if (is_probably_adxl_device):
    freqVal = 125.
    amplVal = 256000.
   else:
    freqVal = 104.
    amplVal = 16384.
   if (not bIncludeTraces() or i_loc == 0 or i_loc == 2 or i_loc >= (maxLength + 5)):
    file_loc.write(   "<table:table-cell table:style-name=\"Default\" table:number-columns-repeated=\"5\"/>"   )
   elif (i_loc == 1):
    file_loc.write(   "<table:table-cell office:value-type=\"float\" office:value=\"2\" calcext:value-type=\"float\"><text:p>2</text:p></table:table-cell>"   )
    file_loc.write(   "<table:table-cell table:style-name=\"Default\" table:number-columns-repeated=\"3\"/>"   )
    file_loc.write(   "<table:table-cell table:style-name=\"Default\" office:value-type=\"float\" office:value=\"%0.1f\" calcext:value-type=\"float\"><text:p>%0.1f</text:p></table:table-cell>"   %  (amplVal, amplVal)  )
   elif (i_loc == 3):
    file_loc.write(   "<table:table-cell table:formula=\"of:=INDEX([.$B$2:.$K$%d];[.$M$3]-1;1)\" office:value-type=\"float\" office:value=\"2\" calcext:value-type=\"float\"><text:p>2</text:p></table:table-cell>"    %  (list_len_loc+1)   )
    file_loc.write(   "<table:table-cell table:style-name=\"ce3\" table:formula=\"of:=INDEX([.$B$2:.$K$%d];[.$M$3]-1;2)\" office:value-type=\"date\" office:date-value=\"2018-04-01T12:55:45\" calcext:value-type=\"date\"><text:p>01/04/2018 12:55:45</text:p></table:table-cell>"   %  (list_len_loc+1)   )
    file_loc.write(   "<table:table-cell/>"  )
    file_loc.write(   "<table:table-cell table:style-name=\"Default\" table:formula=\"of:=INDEX([.$B$2:.$K$%d];[.$M$3]-1;5)\" office:value-type=\"float\" office:value=\"80\" calcext:value-type=\"float\"><text:p>80</text:p></table:table-cell>"                  %  (list_len_loc+1)   )
    file_loc.write(   "<table:table-cell table:style-name=\"Default\" office:value-type=\"float\" office:value=\"%0.0f\" calcext:value-type=\"float\"><text:p>%0.0f</text:p></table:table-cell>" % (freqVal, freqVal)   )
   elif (i_loc == 4):
    file_loc.write(   "<table:table-cell table:style-name=\"Default\" table:number-columns-repeated=\"2\"/>"   )
    file_loc.write(   "<table:table-cell office:value-type=\"string\" calcext:value-type=\"string\"><text:p>X</text:p></table:table-cell>"   )
    file_loc.write(   "<table:table-cell office:value-type=\"string\" calcext:value-type=\"string\"><text:p>Y</text:p></table:table-cell>"   )
    file_loc.write(   "<table:table-cell office:value-type=\"string\" calcext:value-type=\"string\"><text:p>Z</text:p></table:table-cell>"   )
   elif (i_loc < (maxLength + 5)):
    file_loc.write(   "<table:table-cell office:value-type=\"float\" office:value=\"%d\" calcext:value-type=\"float\"><text:p>%d</text:p></table:table-cell>"   %  (i_loc-4, i_loc-4)    )
    file_loc.write(   "<table:table-cell table:formula=\"of:=([.M%d]-1)/[.$Q$5]\" office:value-type=\"float\" office:value=\"%0.4f\" calcext:value-type=\"float\"><text:p>%0.4f</text:p></table:table-cell>" %  (i_loc+2, (i_loc-4)/freqVal, (i_loc-4)/freqVal)   )
    file_loc.write(   "<table:table-cell table:formula=\"of:=INDEX([$Data.$A$1:.$C$%d];[.$M$5]+[.$M%d]+1;1)/[.$Q$3]\" office:value-type=\"float\" office:value=\"0.0000\" calcext:value-type=\"float\"><text:p>0.0000</text:p></table:table-cell>"    %   ((full_len_loc+1), (i_loc+2))    )
    file_loc.write(   "<table:table-cell table:formula=\"of:=INDEX([$Data.$A$1:.$C$%d];[.$M$5]+[.$M%d]+1;2)/[.$Q$3]\" office:value-type=\"float\" office:value=\"0.0000\" calcext:value-type=\"float\"><text:p>0.0000</text:p></table:table-cell>"    %   ((full_len_loc+1), (i_loc+2))    )
    file_loc.write(   "<table:table-cell table:formula=\"of:=INDEX([$Data.$A$1:.$C$%d];[.$M$5]+[.$M%d]+1;3)/[.$Q$3]\" office:value-type=\"float\" office:value=\"0.0000\" calcext:value-type=\"float\"><text:p>0.0000</text:p></table:table-cell>"    %   ((full_len_loc+1), (i_loc+2))    )



  full_len = line_number-1    # Length of data in "Data" table
  list_len = the_index        # Length of list in "List" table


  i = 0   # current line number


  f7 = open(  runningPath +  "/7", "r"  )   # Same file as before, reading back


  for f7_line in f7:
   dest.write(   "<table:table-row table:style-name=\"ro1\">"   )
   dest.write(   f7_line  )
   #dest.write(   "<table:table-cell table:number-columns-repeated=\"12\"/>"    )

   WriteIndexingRow(dest, i, full_len, list_len)
   dest.write(    "</table:table-row>"    )
   i += 1

  while (i <= (maxLength + 5)):
   dest.write(   "<table:table-row table:style-name=\"ro1\">"   )
   dest.write(   "<table:table-cell table:number-columns-repeated=\"12\"/>"    )
   WriteIndexingRow(dest, i, full_len, list_len)
   dest.write(    "</table:table-row>"    )
   i += 1

  dest.write(    "<table:table-row table:style-name=\"ro1\" table:number-rows-repeated=\"15\"><table:table-cell table:number-columns-repeated=\"17\"/></table:table-row><table:table-row table:style-name=\"ro1\"><table:table-cell table:number-columns-repeated=\"17\"/></table:table-row>"        )

  f7.close()


  dest.write(    "</table:table>"     )
  if bAddHeartbeatsTable():
   dest.write(    "<table:table table:name=\"Heartbeats\" table:style-name=\"ta1\">"   )
   dest.write(    "<table:table-column table:style-name=\"co1\" table:number-columns-repeated=\"2\" table:default-cell-style-name=\"Default\"/>"   )
   dest.write(    "<table:table-column table:style-name=\"co2\" table:default-cell-style-name=\"ce3\"/>"   )
   dest.write(    "<table:table-column table:style-name=\"co1\" table:number-columns-repeated=\"7\" table:default-cell-style-name=\"Default\"/>"   )
   dest.write(    "<table:table-row table:style-name=\"ro1\">"  )
   dest.write(       "<table:table-cell/>"     )
   dest.write(       "<table:table-cell office:value-type=\"string\" calcext:value-type=\"string\"><text:p>Row Number</text:p></table:table-cell>"    )
   dest.write(       "<table:table-cell office:value-type=\"string\" calcext:value-type=\"string\"><text:p>Date/Time</text:p></table:table-cell>"     )
   dest.write(       "<table:table-cell office:value-type=\"string\" calcext:value-type=\"string\"><text:p>Item</text:p></table:table-cell>"          )
   dest.write(       "<table:table-cell office:value-type=\"string\" calcext:value-type=\"string\"><text:p>S</text:p></table:table-cell>"             )
   dest.write(       "<table:table-cell office:value-type=\"string\" calcext:value-type=\"string\"><text:p>Tacc (deg C)</text:p></table:table-cell>"  )
   dest.write(       "<table:table-cell office:value-type=\"string\" calcext:value-type=\"string\"><text:p>Tint (deg C)</text:p></table:table-cell>"  )
   dest.write(       "<table:table-cell office:value-type=\"string\" calcext:value-type=\"string\"><text:p>Vbat (V)</text:p></table:table-cell>"      )
   dest.write(       "<table:table-cell office:value-type=\"string\" calcext:value-type=\"string\"><text:p>VCCIO (V)</text:p></table:table-cell>"     )
   dest.write(       "<table:table-cell office:value-type=\"string\" calcext:value-type=\"string\"><text:p>VCCCORE (V)</text:p></table:table-cell>"   )
   dest.write(    "</table:table-row>"   )
   f8 = open(  runningPath +  "/8", "r"  )   # Same file as before, reading back
   for f8_line in f8:
    dest.write(   "<table:table-row table:style-name=\"ro1\">"   )
    dest.write(   f8_line  )
    dest.write(   "</table:table-row>"   )
   f8.close()
   dest.write(    "</table:table>"   )


  dest.write(    "<table:named-expressions/></office:spreadsheet></office:body></office:document-content>")


 ##### Now write to the ZIP archive  ####
 shutil.copy2(  runningPath + "/template_content/3_1.ods", theName)
 with zipfile.ZipFile(theName, "a") as z:
  z.write(  runningPath +  "/content_local.xml", "content.xml", zipfile.ZIP_DEFLATED )
  z.close()

 os.remove(  runningPath +  "/content_local.xml")

 f9.write( "Completed!\r\r" )