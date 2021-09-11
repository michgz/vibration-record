'''
Post-process the output of the vibration-record recorder.
'''


import matplotlib.pyplot as plt
from matplotlib import dates
import datetime
import numpy
import scipy.signal
import sys


INPUT_FILE = ""  # to be filled






def ReadFile(fn, DateTarget=None, DateReference=None):
  
  
  a = []
  
  with open(fn, "r") as f1:
    
      state = 0
      d = {}
      dt = None
      vals_x = []
      vals_y = []
      vals_z = []
    
      for line in f1:
          if state == 0:
              try:
                  dt = datetime.datetime.strptime(line.strip(), "%d/%m/%Y,%H:%M:%S,")
                  if DateTarget is not None and DateReference is not None:
                      dt += (DateTarget - DateReference)
                  state = 1
              except ValueError:
                  pass
          elif state == 1:
              c = line.strip().split(" ")
              for cc in c:
                  # Parse a <keyword>=<value> pair
                  if cc.find("=") >= 0:
                      x = cc.find("=")
                      d.update({cc[0:x]: cc[x+1:]})
              state = 2
          elif state == 2:
              ss = line.strip()
              if ss == "ON":
                  a.append({'datetime': dt, 'args': d, 'type': "ON"})
                  d = {}
                  dt = None
                  vals_x = []
                  vals_y = []
                  vals_z = []

              elif ss == "HEARTBEAT":
                  a.append({'datetime': dt, 'args': d, 'type': "HEARTBEAT"})
                  d = {}
                  dt = None
                  vals_x = []
                  vals_y = []
                  vals_z = []

              else:
                  state = 3
          elif state == 3:
              c = line.strip().split(",")
              if len(c) != 3:
                  # Error! Reset
                  state = 0
              else:
                  try:
                      dt_temp = datetime.datetime.strptime(line.strip(), "%d/%m/%Y,%H:%M:%S,")
                      if DateTarget is not None and DateReference is not None:
                          dt_temp += (DateTarget - DateReference)
                      a.append({'datetime': dt, 'args': d, 'type': "TRACE", 'x': numpy.array(vals_x, dtype='float'),
                                                                            'y': numpy.array(vals_y, dtype='float'),
                                                                            'z': numpy.array(vals_z, dtype='float')})
                      d = {}
                      dt = dt_temp
                      vals_x = []
                      vals_y = []
                      vals_z = []
                      state = 1
                  except ValueError:
                    
                      vals_x.append(float(c[0]))
                      vals_y.append(float(c[1]))
                      vals_z.append(float(c[2]))
                    


                  

                  

  return a
  
  










def calculate_rms(a):
  
    if a['type'] != 'TRACE':
        return 0.
  
    tot = 0.
    tot += numpy.mean(numpy.power(  a['x'] - numpy.mean(a['x'])  , 2.0  ))
    tot += numpy.mean(numpy.power(  a['y'] - numpy.mean(a['y'])  , 2.0  ))
    tot += numpy.mean(numpy.power(  a['z'] - numpy.mean(a['z'])  , 2.0  ))
    return numpy.sqrt(tot)
    
    


def calculate_filtered_rms(a):
  
    FILTER_CUTOFF = 20.  # Hz
    
    f = scipy.signal.bessel(6, FILTER_CUTOFF, analog=False, output='sos', norm='mag', fs=125.)
  
    if a['type'] != 'TRACE':
        return 0.
  
    tot = 0.
    
    i = a['x'] - numpy.mean(a['x'])
    j = scipy.signal.sosfilt(f, i)
    tot += numpy.mean(numpy.power(  j  , 2.0  ))

    i = a['y'] - numpy.mean(a['y'])
    j = scipy.signal.sosfilt(f, i)
    tot += numpy.mean(numpy.power(  j  , 2.0  ))

    i = a['z'] - numpy.mean(a['z'])
    j = scipy.signal.sosfilt(f, i)
    tot += numpy.mean(numpy.power(  j  , 2.0  ))

    return numpy.sqrt(tot)
    
    



def calculate_frequency(a):

    if a['type'] != 'TRACE':
        return 0.

    # Use the "X" axis for this. It just happens that tends to be strongest
    # in the example data.
    
    j = scipy.signal.periodogram(a['x'], fs=125.)
    
    return (j[0][  numpy.argmax(j[1])  ])




PLOT_TEMPERATURE = False
PLOT_CORRELATIONS = False   # Used to try to "patchwork" together overlapping traces






aa = ReadFile(INPUT_FILE,
                  datetime.datetime.strptime("07/09/2021 21:08:00", "%d/%m/%Y %H:%M:%S"),
                  datetime.datetime.strptime("06/01/2048 16:39:12", "%d/%m/%Y %H:%M:%S"))



with open("TIMES.csv", "w") as f2:
  
    last_dt = None
    last_i = None
    for i, aaa in enumerate(aa):
        if aaa['type'] == 'TRACE':
            if not last_dt is None:
                f2.write(   last_dt.strftime("%d/%m/%Y %H:%M:%S")  + ",{0}\n".format(  (aaa['datetime']-last_dt).seconds        ))
                
                
                if PLOT_CORRELATIONS and (aaa['datetime']-last_dt).seconds < 10:
                  
                    y = scipy.signal.correlate(  aa[last_i]['x'], aa[i]['x'], mode='full' )
                    x = numpy.linspace( 0, float(len(y))/125., len(y)  )
                  
                    plt.plot( x,y )
                    plt.title(   (aaa['datetime']-last_dt).seconds    )
                    plt.show()
                
                
            last_dt = aaa['datetime']
            last_i = i

            
            


with open("TOTAL_RMS.csv", "w") as f1:
  
    for aaa in aa:
        f1.write(  aaa['datetime'].strftime("%d/%m/%Y %H:%M:%S")  + ",{0}".format(calculate_filtered_rms(aaa)))
        f1.write( ",{0}\n".format(   calculate_frequency(aaa)  ))




idx = 17   # a "nice" example signal


calculate_frequency(aa[17])
sys.exit(0)


if False:

    f = scipy.signal.bessel(6, 20, analog=False, output='sos', norm='mag', fs=125)


    i = aa[idx]['x']
    j = scipy.signal.sosfilt(f, i)

    u = numpy.linspace( 0, float(len(i))/125., len(i))

    plt.plot(u,i)
    plt.plot(u,j)
    plt.show()



    sys.exit(0)




idx = 191  # a "bad" example (want to reject)



if False:


    f = scipy.signal.bessel(6, 20, analog=False, output='sos', norm='mag', fs=125)


    i = aa[idx]['y']
    j = scipy.signal.sosfilt(f, i)

    u = numpy.linspace( 0, float(len(i))/125., len(i))

    plt.plot(u,i)
    plt.plot(u,j)
    plt.show()



    sys.exit(0)





if PLOT_TEMPERATURE:


    x = []
    y = []
    for aaa in aa:
        if aaa['type'] == 'HEARTBEAT' or aaa['type'] == 'ON':
            x.append(aaa['datetime'])
            y.append(aaa['args']['Text'])
    
    plt.axes().xaxis.set_major_formatter(dates.DateFormatter('%H:%M'))
    
    plt.plot(x,y,'.-')
    
    #plt.axes().xaxis.set_major_locator(dates.HourLocator(2))

    plt.show()


else:

    for j, aaa in enumerate(aa):
        if aaa['type'] == 'TRACE':
            plt.plot(aaa['x'])
            plt.plot(aaa['y'])
            plt.plot(aaa['z'])
            #plt.title(aaa['datetime'].strftime("%d/%m/%Y %H:%M:%S"))
            plt.title("{0}".format(  calculate_filtered_rms(aaa)  ))
            #plt.title(j)
            plt.show()



