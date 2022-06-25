/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
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

// CRASH when enabled, with IPlug2
#if 0 // #bl-iplug2
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
#endif
