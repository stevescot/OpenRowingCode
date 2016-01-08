char standards[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.~"; /* String containing chars you want encoded */

static char hex_digit(unsigned char c)
{  return "01234567890ABCDEF"[c & 0x0F];
}

char *urlencode(char *dst,const char *src)
{  char c,*d = dst;
   while (c = *src++)
   {  if (!strchr(standards,c))
      {  *d++ = '%';
         *d++ = hex_digit(c >> 4);
         *d++ = hex_digit(c);
      }
      else *d++ = c;
   }
   return dst;
}

char *urlencode(char*dst, const String src)
{
  return urlencode(dst, src.c_str());
}

