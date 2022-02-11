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
