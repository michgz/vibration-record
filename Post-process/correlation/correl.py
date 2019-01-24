
import datetime


adxl = []
with open('ADXL.csv','r') as f1:
 for s in f1:
  t = s.strip().split('\t')
  a1 = datetime.datetime.strptime(t[0], '%d/%m/%Y %H:%M:%S')
  a2 = float(t[1])
  a3 = float(t[2])
  adxl.append([a1,a2,a3])

lsm6 = []
with open('LSM6.csv','r') as f1:
 for s in f1:
  t = s.strip().split('\t')
  a1 = datetime.datetime.strptime(t[0], '%d/%m/%Y %H:%M:%S')
  a2 = float(t[1])
  a3 = float(t[2])
  lsm6.append([a1,a2,a3])





print len(adxl)
print len(lsm6)
