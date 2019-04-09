
import sys
import os
import mdct
import numpy
import scipy
import datetime


## A "squelch" term. Omits samples with peak acceleration less than this (units: m s^-2)
#  Set to 0. to disable

Lowest_val = 0.



## Calculate square of a number
def sq(x):
 return 1.*x*x


class Trace:
 x = []
 y = []
 z = []
 data_pos = 0
 device_type = ""
 date_time = datetime.datetime(2000, 1, 1, 0, 0)

 def SetDateTime(self,dt):
    self.date_time = dt

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


 def MaxAxis(self):
  if self.data_pos < 10:
   return -1
  else:
   num = self.data_pos
   sum = [0., 0., 0.]
   for j in range(num):
    sum[0] += self.x[j]
    sum[1] += self.y[j]
    sum[2] += self.z[j]
   avg = [sum[0]/num, sum[1]/num, sum[2]/num]
   maxdev = [0., 0., 0.]
   for j in range(num):
    if (abs(self.x[j]-avg[0]) > maxdev[0]):
     maxdev[0]= abs(self.x[j]-avg[0])
    if (abs(self.y[j]-avg[1]) > maxdev[1]):
     maxdev[1]= abs(self.y[j]-avg[1])
    if (abs(self.z[j]-avg[2]) > maxdev[2]):
     maxdev[2]= abs(self.z[j]-avg[2])
   if (maxdev[2] >= maxdev[1]) and (maxdev[2] >= maxdev[0]):
    return 2
   elif (maxdev[1] >= maxdev[0]):
    return 1
   else:
    return 0


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



def power4(uu):
    temp = uu*uu
    return temp*temp

def power2(uv):
    return uv*uv


def MaxPos(vec1):
    if (len(vec1) < 10):
        return -1
    else:
        sum = 0.
        for j in range(len(vec1)):
            sum += vec1[j]
        avg = sum / len(vec1)
        maxdev = 0.
        maxdevpos = -1
        for j in range(len(vec1)):
            if (vec1[j] > maxdev):
                maxdev = vec1[j]
                maxdevpos = j
        return j



def CorrectionFactorXY(fHz):
    if (fHz <= 2.):
        return 3.57E-3
    else:
        return 3.57E-3 * numpy.exp(     numpy.log(fHz) - numpy.log(2.)   )


def CorrectionFactorZ(fHz):
    if (fHz <= 4.):
        return 5.0E-3 * numpy.exp(     -0.5*(   numpy.log(fHz) - numpy.log(4.)   )   )
    elif (fHz <= 8.):
        return 5.0E-3
    else:
        return 5.0E-3 * numpy.exp(     numpy.log(fHz) - numpy.log(8.)   )







def CorrectionFactor(fHz,Axis):
    if (Axis==2):
        return CorrectionFactorZ(fHz)/5.0E-3
    else:
        return CorrectionFactorXY(fHz)/3.57E-3



def ProcessTrace(trace1):
    global day_sum4
    global night_sum4
    # print trace1.data_pos
    if (trace1.data_pos < 99):
        return
    an = trace1.MaxAxis()
    if (an == 0):
        v = trace1.x
    elif (an == 1):
        v = trace1.y
    else:
        v = trace1.z


    sum = 0.
    maxval_1 = 0.
    maxvalpos_1 = -1
    for j in range(len(v)):
        sum += v[j]
    avg = sum / len(v)
    for j in range(len(v)):
        v[j] -= avg
        if (abs(v[j]) > maxval_1):
            maxval_1 = abs(v[j])
            maxvalpos_1 = j
    nn = maxvalpos_1
    if (len(v) < 80):
        return   # Not enough data
    if (nn < 40):
        nn = 40
    elif (nn > len(v)-40):
        nn = len(v)-40
    w = mdct.mdct(v[nn-40:nn+39], window=scipy.signal.boxcar, framelength = 64)
    if (w.shape[1] != 5):
        print "Whoops! Should be 5 columns wide"
        sys.exit(-1)
    maxval = 0.
    maxvalpos = -1
    for j in range(w.shape[0]):
        q = abs(w[j,2])
        if (q > maxval):
            maxval = q
            maxvalpos = j
    freq = (maxvalpos + 0.5)*125/64    # Hz
    avgs = [0., 0., 0.]
    sums = [0., 0., 0.]
    for j in range(trace1.data_pos):
        sums[0] += trace1.x[j]
        sums[1] += trace1.y[j]
        sums[2] += trace1.z[j]
    avgs[0] = sums[0] / trace1.data_pos
    avgs[1] = sums[1] / trace1.data_pos
    avgs[2] = sums[2] / trace1.data_pos
    sum4 = 0.
    max2 = 0.
    for j in range(trace1.data_pos):
        val2 = (power2(trace1.x[j] - avgs[0]) + power2(trace1.y[j] - avgs[1]) + power2(trace1.z[j] - avgs[2]) )
        if (val2 > max2):
            max2 = val2
        sum4 += power2(val2)
    sum4 *= power4( 9.80665 / 16384. )  /  125    #  Now in S.I. units (m^4/s)
    max2 = (9.80665 / 16384. ) * numpy.sqrt( max2 )   # Now in S.I. units (m/s^2)
    if (freq < 1.0):
        used_freq = 20.0
    elif (freq > 80.0):
        used_freq = 20.0
    else:
        used_freq = freq
    correction = CorrectionFactor(used_freq, an)
    sum4 /= correction
    
    ## Accumulate into the global variables if deemed big enough
    if (max2 >= Lowest_val):
        if (trace1.date_time.hour >= 7) and (trace1.date_time.hour < 23):
            day_sum4 += sum4
        else:
            night_sum4 += sum4
    with open('Outs.CSV', 'a') as f2:
        f2.write(   trace1.date_time.strftime('%d/%m/%Y %H:%M:%S')   )
        f2.write(  ',%d,%d,%e,%f,%d' % (len(v), an, max2, maxval, maxvalpos)   )
        f2.write(  ',%f,%f,%e'   %  (freq, correction, sum4)    )
        f2.write('\n')


try:
    os.remove("Outs.CSV")
except:
    pass
    
with open('Outs.CSV', 'a') as f2:
    f2.write(   'Date/Time,Sample length,Vibration axis,Maximum [m s^-2],MDCT maximum value,MDCT maximum index,'   )
    f2.write(   'Frequency [Hz],Correction factor,Integral [m^4 s^-8]\n'   )
with open('Raw/20190406.CSV', 'r') as f1:
    i = 0
    j = 9999
    day_sum4 = 0.
    night_sum4 = 0.
    current_trace = Trace()
    for line in f1:
        if (line[2:10]=='/04/2019'):
            j = 0
            # print '(%s -- %d)' % (line.strip(), i)
            ProcessTrace(current_trace)
            current_trace.Clear()
            current_trace.SetDateTime(   datetime.datetime.strptime(line.strip(), '%d/%m/%Y,%H:%M:%S,' )   )
        else:
            if (j < 9000):
                j += 1
                if (j >= 2):
                    t = line.split(',')
                    if (len(t) == 3):
                        current_trace.Add([float(t[0]), float(t[1]), float(t[2])])
with open('Outs.CSV', 'a') as f2:
    f2.write(   ',,,,,,,,\n'                                             )
    f2.write(   ',,,,,,,Day:,%e\n'     %   numpy.power(day_sum4, 0.25)     )
    f2.write(   ',,,,,,,Night:,%e\n'   %   numpy.power(night_sum4, 0.25)   )
