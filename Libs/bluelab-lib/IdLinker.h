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
 
#ifndef ID_LINKER_H
#define ID_LINKER_H

#include <vector>
using namespace std;

template<class T0, class T1>
class IdLinker
{
 public:
    static void LinkIds(vector<T0> *t0Vec, vector<T1> *t1Vec, bool needSort)
    {
        if (needSort)
        {
            sort(t0Vec->begin(), t0Vec->end(), T0::IdLess);
            sort(t1Vec->begin(), t1Vec->end(), T1::IdLess);
        }
        
        // Init
        for (int i = 0; i < t0Vec->size(); i++)
            (*t0Vec)[i].mLinkedId = -1;
        for (int i = 0; i < t1Vec->size(); i++)
            (*t1Vec)[i].mLinkedId = -1;

        int i0 = 0;
        int i1 = 0;
        while(true)
        {
            if (i0 >= t0Vec->size())
                return;
            if (i1 >= t1Vec->size())
                return;
        
            T0 &t0 = (*t0Vec)[i0];
            T1 &t1 = (*t1Vec)[i1];

            if (t0.mId == t1.mId)
            {
                t0.mLinkedId = i1;
                t1.mLinkedId = i0;

                i0++;
                i1++;
            
                continue;
            }
            
            if (t0.mId > t1.mId)
                i1++;

            if (t0.mId < t1.mId)
                i0++;
        }
    }
};

#endif
