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
//  UnstableRemoveIf.h
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/27/20.
//
//

#ifndef BL_SoundMetaViewer_UnstableRemoveIf_h
#define BL_SoundMetaViewer_UnstableRemoveIf_h

// See: https://www.gamedev.net/forums/topic/403104-faster-stl-remove_if/

template <typename ForwardItor, typename Pred>
ForwardItor unstable_remove_if(ForwardItor first, ForwardItor last, Pred pred)
{
    while(first != last)
    {
        if(pred(*first))
            swap(*first, *--last);
        else
            ++first;
    }
    return last;
}

#if 0
template <typename ForwardItor, typename T>ForwardItor
unstable_remove(ForwardItor first, ForwardItor last, const T &value)
{
    return unstable_remove_if(first, last, std::bind2nd(std::equal_to<T>(), value));
}
#endif

// Niko
template <typename ForwardItor, typename Pred, typename Arg>
ForwardItor unstable_remove_if(ForwardItor first, ForwardItor last, Pred pred, Arg arg)
{
    while(first != last)
    {
        if(pred(*first, arg))
            swap(*first, *--last);
        else
            ++first;
    }
    return last;
}

#if 0 // Other version
template <typename BidirectionalIterator, typename Predicate>
BidirectionalIterator unstable_remove_if(BidirectionalIterator first, BidirectionalIterator last, Predicate pred)
{
    BidirectionalIterator cursor = first;
    while (cursor != last)
    {
        if (pred(*cursor))
        {
            --last;
            if (cursor != last)
            {
                using std::swap;
                swap(*cursor, *last);
            }
        }
        else
        {
            ++cursor;
        }
    }
    return last;
}
#endif

#endif
