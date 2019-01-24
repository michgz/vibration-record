
import datetime
import os






adxl = []
with open(os.path.join(os.path.dirname(os.path.abspath(__file__)),'ADXL.csv'),'r') as f1:
 for s in f1:
  t = s.strip().split('\t')
  a1 = datetime.datetime.strptime(t[0], '%d/%m/%Y %H:%M:%S')
  a2 = float(t[1])
  a3 = float(t[2])
  adxl.append([a1,a2,a3])

lsm6 = []
with open(os.path.join(os.path.dirname(os.path.abspath(__file__)),'LSM6.csv'),'r') as f1:
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

