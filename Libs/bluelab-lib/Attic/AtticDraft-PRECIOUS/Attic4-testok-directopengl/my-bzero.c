//
//  my-bzero.c
//  Transient
//
//  Created by Apple m'a Tuer on 06/09/17.
//
//

#include <stdio.h>
#include <string.h>

#include "my-bzero.h"

void
bzero(void *s, size_t n)
{
    memset(s, '\0', n);
}

void
_bzero(void *s, size_t n)
{
    memset(s, '\0', n);
}

void
__bzero(void *s, size_t n)
{
    memset(s, '\0', n);
}

void
___bzero(void *s, size_t n)
{
    memset(s, '\0', n);
}

void
____bzero(void *s, size_t n)
{
    memset(s, '\0', n);
}