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
//  StlInsertSorted.h
//  BL-SoundMetaViewer
//
//  Created by applematuer on 4/11/20.
//
//

#ifndef BL_SoundMetaViewer_StlInsertSorted_h
#define BL_SoundMetaViewer_StlInsertSorted_h

#include <vector>

template< typename T, typename Pred >
typename std::vector<T>::iterator
insert_sorted( std::vector<T> & vec, T const& item, Pred pred )
{
    return vec.insert
    (
     std::upper_bound( vec.begin(), vec.end(), item, pred ),
     item
     );
}

#endif
