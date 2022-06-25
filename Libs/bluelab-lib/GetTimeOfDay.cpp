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
 
#ifdef WIN32

// From http://mathieuturcotte.ca/textes/windows-gettimeofday

#include <windows.h>

#include <GetTimeOfDay.h>

#ifdef __cplusplus
extern "C" {
#endif

    int gettimeofday(struct timeval* p, void* tz) {
        ULARGE_INTEGER ul; // As specified on MSDN.
        FILETIME ft;

        // Returns a 64-bit value representing the number of
        // 100-nanosecond intervals since January 1, 1601 (UTC).
        GetSystemTimeAsFileTime(&ft);

        // Fill ULARGE_INTEGER low and high parts.
        ul.LowPart = ft.dwLowDateTime;
        ul.HighPart = ft.dwHighDateTime;
        // Convert to microseconds.
        ul.QuadPart /= 10ULL;
        // Remove Windows to UNIX Epoch delta.
        ul.QuadPart -= 11644473600000000ULL;
        // Modulo to retrieve the microseconds.
        p->tv_usec = (long)(ul.QuadPart % 1000000LL);
        // Divide to retrieve the seconds.
        p->tv_sec = (long)(ul.QuadPart / 1000000LL);

        return 0;
    }

#ifdef __cplusplus
}
#endif

#endif
