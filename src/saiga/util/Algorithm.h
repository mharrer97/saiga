/**
 * Copyright (c) 2017 Darius Rückert
 * Licensed under the MIT License.
 * See LICENSE file for more information.
 */

#pragma once

#include "saiga/config.h"

namespace Saiga
{
/**
 * Computes the exclusive scan into 'output' and returns the total sum.
 */
template <typename _InputIterator1, typename _InputIterator2, typename _Tp>
inline _Tp exclusive_scan(_InputIterator1 __first1, _InputIterator1 __last1, _InputIterator2 __output, _Tp __init)
{
    for (; __first1 != __last1; ++__first1, (void)++__output)
    {
        *__output = __init;
        __init    = __init + *__first1;
    }
    return __init;
}


}  // namespace Saiga
