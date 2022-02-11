#include <errno.h>
//#include <libio.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#define _IO_off64_t long long int
#define  ssize_t long long int

typedef struct fmemopen_cookie_struct fmemopen_cookie_t;
struct fmemopen_cookie_struct
{
  char *buffer;
  int mybuffer;
  int binmode;
  size_t size;
  _IO_off64_t pos;
  size_t maxpos;
};


static ssize_t
fmemopen_read (void *cookie, char *b, size_t s)
{
  fmemopen_cookie_t *c;

  c = (fmemopen_cookie_t *) cookie;

  if (c->pos + s > c->size)
    {
      if ((size_t) c->pos == c->size)
        return 0;
      s = c->size - c->pos;
    }

  memcpy (b, &(c->buffer[c->pos]), s);

  c->pos += s;
  if ((size_t) c->pos > c->maxpos)
    c->maxpos = c->pos;

  return s;
}


static ssize_t
fmemopen_write (void *cookie, const char *b, size_t s)
{
  fmemopen_cookie_t *c;
  int addnullc;

  c = (fmemopen_cookie_t *) cookie;

  addnullc = c->binmode == 0 && (s == 0 || b[s - 1] != '\0');

  if (c->pos + s + addnullc > c->size)
    {
      if ((size_t) (c->pos + addnullc) == c->size)
        {
          errno=(ENOSPC);
          return 0;
        }
      s = c->size - c->pos - addnullc;
    }

  memcpy (&(c->buffer[c->pos]), b, s);

  c->pos += s;
  if ((size_t) c->pos > c->maxpos)
    {
      c->maxpos = c->pos;
      if (addnullc)
        c->buffer[c->maxpos] = '\0';
    }

  return s;
}


static int
fmemopen_seek (void *cookie, _IO_off64_t *p, int w)
{
  _IO_off64_t np;
  fmemopen_cookie_t *c;

  c = (fmemopen_cookie_t *) cookie;

  switch (w)
    {
    case SEEK_SET:
      np = *p;
      break;

    case SEEK_CUR:
      np = c->pos + *p;
      break;

    case SEEK_END:
      np = (c->binmode ? c->size : c->maxpos) - *p;
      break;

    default:
      return -1;
    }

  if (np < 0 || (size_t) np > c->size)
    return -1;

  *p = c->pos = np;

  return 0;
}


static int
fmemopen_close (void *cookie)
{
  fmemopen_cookie_t *c;

  c = (fmemopen_cookie_t *) cookie;

  if (c->mybuffer)
    free (c->buffer);
  free (c);

  return 0;
}


FILE *
fmemopen (void *buf, size_t len, const char *mode)
{
  cookie_io_functions_t iof;
  fmemopen_cookie_t *c;

  if (__builtin_expect (len == 0, 0))
    {
    einval:
      errno=(EINVAL);
      return NULL;
    }

  c = (fmemopen_cookie_t *) malloc (sizeof (fmemopen_cookie_t));
  if (c == NULL)
    return NULL;

  c->mybuffer = (buf == NULL);

  if (c->mybuffer)
    {
      c->buffer = (char *) malloc (len);
      if (c->buffer == NULL)
        {
          free (c);
          return NULL;
        }
      c->buffer[0] = '\0';
    }
  else
    {
      if (__builtin_expect ((uintptr_t) len > -(uintptr_t) buf, 0))
        {
          free (c);
          goto einval;
        }

      c->buffer = buf;
    }

  c->size = len;

  if (mode[0] == 'w')
    c->buffer[0] = '\0';

  c->maxpos = strlen (c->buffer);

  if (mode[0] == 'a')
    c->pos = c->maxpos;
  else
    c->pos = 0;

  c->binmode = mode[0] != '\0' && mode[1] == 'b';

  iof.read = fmemopen_read;
  iof.write = fmemopen_write;
  iof.seek = fmemopen_seek;
  iof.close = fmemopen_close;

  return fopencookie (c, mode, iof);
}


