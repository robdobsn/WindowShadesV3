// StringFormat
// Rob Dobson 2012-2018

#pragma once

class StringFormat
{
  public:
    static String format(const char* fmt, ...)
    {
      va_list marker;
      va_start(marker, fmt);
      const int bufsize = 5;
      char test[bufsize];
      size_t n = vsnprintf(test, bufsize, fmt, marker);
      va_end(marker);

      String result;
      char* buf = new char[n+1];
      if (buf) {
          va_start(marker, fmt);
          n = vsnprintf(buf, n+1, fmt, marker);
          va_end(marker);
          result = buf;
      }
      delete [] buf;
      return result;
    }
};

