

#include "ConfigurationFile.h"




///////////////////////////////////////////////////////////////////////////////
/////////// Configuration File reading  ///////////////////////////////////////


// The configuration file is named "setup.txt" (note lowercase), and on startup
//  it is read an renamed to "setupold.txt". Therefore it will only be used once.



static int sdReadLnN(File f, char * buf, int len)
{
  bool loop = true;
  int result = 0;
  while (loop)
  {
    signed char d = f.read();
    if (d >= 0x10)
    {
      if (len > 0)
      {
        *buf = d;
        buf ++;
        len --;
        result ++;
      }
    }
    else if (result > 0 || d < 0)
    {
      loop = false;
    }
  }
  return result;
}


bool sdReadConfigPair(File f, char * buf1, int len1, char * buf2, int len2)
{
  char c [100];
  int h;

  h = sdReadLnN(f, c, 98);
  if (h)
  {
    char * m = strstr(c, "="); // find the first occurance of an equals
    if (!m)
    {
      return false;
    }
    int i1 = m - c;

    if (i1 > len1)
    {
      i1 = len1;
    }
    strncpy(buf1, c, i1);
    if (i1 < len1)
    {
      buf1[i1] = '\0';
    }
    else
    {
      buf1[len1 - 1] = '\0'; // Ensure null-terminated
    }

    m ++;
    int i2 = strlen(m);
    if (i2 > len2)
    {
      i2 = len2;
    }
    strncpy(buf2, m, i2);
    if (i2 < len2)
    {
      buf2[i2] = '\0';
    }
    else
    {
      buf2[len2 - 1] = '\0'; // Ensure null-terminated
    }

    if (i1 > 0 && i2 > 0)
    {
      return true;
    }

  }

  return false;


}


void sdCopyFile(String nameFrom, String nameTo)
{
  File from, to;
  from = SD.open(nameFrom, FILE_READ);
  if (from)
  {
    if (SD.exists(nameTo))
    {
      SD.remove(nameTo);
    }
    to = SD.open(nameTo, FILE_WRITE);
    if (to)
    {
      signed char Byte = 0;
      while (Byte >= 0)
      {
        Byte = from.read();
        if (Byte >= 0)
        {
          to.write(Byte);
        }
      }
      to.close();
    }
    from.close();
  }
}
