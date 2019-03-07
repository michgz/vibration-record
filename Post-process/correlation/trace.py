import datetime
import numpy

## Calculate square of a number
def sq(x):
 return 1.*x*x

class Trace:
 x = []
 y = []
 z = []
 dt = datetime.datetime(2017, 1, 1, 0, 0)
 data_pos = 0
 device_type = ""

 def copy(self):
  copy_trace = Trace()
  copy_trace.x = self.x
  copy_trace.y = self.y
  copy_trace.z = self.z
  copy_trace.data_pos = self.data_pos
  copy_trace.dt = self.dt
  return copy_trace

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



##
# Helper function
#
def IsLikelyDateTime(str):
 if len(str) < 16:
  return False
 if str[2] == '/' and str[5] == '/':
  return True
 return False



def bIsLineNumberOkay():
 return False



##
#  A function to read in a single file and return the traces contained in it.
#

def ReadIn(inFiles):
  all_traces = []
  current_trace = Trace()
  current_trace_line_number = 0
  current_trace_dt = datetime.datetime(2017, 1, 1, 0, 0)
  current_header_line = ""
  a = 0
  the_index = 0
  the_row = 0
  line_number = 1
  maxLength = 0
  for x in inFiles:
   with open(x, 'r') as f1:
    for line in f1:
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
         if (current_trace.data_pos > maxLength):
          maxLength = current_trace.data_pos
         all_traces.append(current_trace.copy())
         the_index += 1

        # Start the new trace
        current_trace.Clear()
        if bIsLineNumberOkay():
         current_trace_line_number = line_number
        else:
         current_trace_line_number = -1
        current_trace_dt = dt
        current_trace.dt = dt
        current_trace.device_type = ""
        the_index += 1
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
       try:
        currentTriggerVal = float(line[line.find("C=")+2:].split(' ')[0])
       except ValueError:
        currentTriggerVal = -1.
       current_header_line = line.strip()  # Only used if next line is "HEARTBEAT"
       the_row += 1
      elif (a==2) and (line.find("HEARTBEAT") >= 0 or line.find("ON") >= 0):
       # It's a heartbeat line. Leave.
       the_row += 1
       a += 1
      else:
       d = line.split(',')
       d_num = [0, 0, 0]
       if (len(d) >= 1):
        try:
         d_num[0] = float(d[0].strip())
        except ValueError:
         pass
       if (len(d) >= 2):
        try:
         d_num[1] = float(d[1].strip())
        except ValueError:
         pass
       if (len(d) >= 3):
        try:
         d_num[2] = float(d[2].strip())
        except ValueError:
         pass
        current_trace.Add(d_num)
       the_row += 1
       a += 1
     if bIsLineNumberOkay():
      line_number += 1

  if (current_trace.data_pos >= 250):
   # Flush the final trace
   if (current_trace.data_pos > maxLength):
    maxLength = current_trace.data_pos
   all_traces.append(current_trace.copy())
   the_index += 1
  return all_traces

