# Run with Python 2.7
##


import zipfile
import shutil
import numpy
import os
import sys
import datetime
import getopt



#### Set this variable to True if only a tabulated summary output is *NOT* required #####
bIsShort = True

currentTriggerVal = 0

def bIncludeTraces():
 return not bIsShort

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
 file.write(   "<table:table-cell office:value-type=\"float\" office:value=\"%d\" calcext:value-type=\"float\"><text:p>%d</text:p></table:table-cell>" % (currentTriggerVal, currentTriggerVal)   )
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


 
 
def dosage_spreadsheet(*args, **kwargs):
    with open("out.csv", "w") as f1:
        f1.write("Hello\r\n")





#######################################
# Now handle the command-line syntax.
#######################################





def usage():
    print "-h  help"
    print "-o  output"


def main():
    try:
        opts, args = getopt.gnu_getopt(sys.argv[1:], "ho:", ["help", "output="])
    except getopt.GetoptError as err:
        # print help information and exit:
        print str(err)  # will print something like "option -a not recognized"
        usage()
        sys.exit(2)
    Output = None
    Long = False
    for o, a in opts:
        if o in ("-h", "--help"):
            usage()
            sys.exit()
        elif o in ("-o", "--output"):
            Output = a
        else:
            assert False, "unhandled option"
    if (Output == None):
        # Have no output, cannot do anythin
        sys.exit(3)
    postproc_spreadsheet(args, output=Output)


if __name__=="__main__":
    main()

