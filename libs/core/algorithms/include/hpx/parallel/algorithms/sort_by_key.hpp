//  Copyright (c) 2016 Hartmut Kaiser
//  Copyright (c) 2016 John Biddiscombe
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/sort_by_key.hpp

#if defined(DOXYGEN)

namespace hpx { namespace parallel { namespace v1 {
    // clang-format off
    /// Sorts one range of data using keys supplied in another range.
    /// The key elements in the range [key_first, key_last) are sorted in
    /// ascending order with the corresponding elements in the value range
    /// moved to follow the sorted order.
    /// The algorithm is not stable, the order of equal elements is not guaranteed
    /// to be preserved.
    /// The function uses the given comparison function object comp (defaults
    /// to using operator<()). Executed according to the policy.
    ///
    /// \note   Complexity: O(Nlog(N)), where N = std::distance(first, last)
    ///                     comparisons.
    ///
    /// A sequence is sorted with respect to a comparator \a comp
    /// if for every iterator i pointing to the sequence and
    /// every non-negative integer n such that i + n is a valid iterator
    /// pointing to an element of the sequence, and
    /// INVOKE(comp, *(i + n), *i) == false.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it applies user-provided function objects.
    /// \tparam KeyIter     The type of the key iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     random access iterator.
    /// \tparam ValueIter   The type of the value iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     random access iterator.
    /// \tparam Compare     The type of the function/function object to use
    ///                     (deduced).
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param key_first    Refers to the beginning of the sequence of key
    ///                     elements the algorithm will be applied to.
    /// \param key_last     Refers to the end of the sequence of key elements the
    ///                     algorithm will be applied to.
    /// \param value_first  Refers to the beginning of the sequence of value
    ///                     elements the algorithm will be applied to, the range
    ///                     of elements must match [key_first, key_last)
    /// \param comp         comp is a callable object. The return value of the
    ///                     INVOKE operation applied to an object of type Comp,
    ///                     when contextually converted to bool, yields true if
    ///                     the first argument of the call is less than the
    ///                     second, and false otherwise. It is assumed that comp
    ///                     will not apply any non-constant function through the
    ///                     dereferenced iterator.
    ///
    /// \a comp has to induce a strict weak ordering on the values.
    ///
    /// The application of function objects in parallel algorithm
    /// invoked with an execution policy object of type
    /// \a sequenced_policy execute in sequential order in the
    /// calling thread.
    ///
    /// The application of function objects in parallel algorithm
    /// invoked with an execution policy object of type
    /// \a parallel_policy or \a parallel_task_policy are
    /// permitted to execute in an unordered fashion in unspecified
    /// threads, and indeterminately sequenced within each thread.
    ///
    /// \returns  The \a sort_by_key algorithm returns a
    ///           \a hpx::future<sort_by_key_result<KeyIter,ValueIter>>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and returns \a
    ///           \a sort_by_key_result<KeyIter,ValueIter> otherwise.
    ///           The algorithm returns a pair holding an iterator pointing to
    ///           the first element after the last element in the input key
    ///           sequence and an iterator pointing to the first element after
    ///           the last element in the input value sequence.
    template <typename ExPolicy, typename KeyIter, typename ValueIter,
        typename Compare = detail::less>
    util::detail::algorithm_result_t<ExPolicy,
        sort_by_key_result<KeyIter, ValueIter>>
    sort_by_key(ExPolicy&& policy, KeyIter key_first, KeyIter key_last,
        ValueIter value_first, Compare&& comp = Compare());
    // clang-format on
}}}    // namespace hpx::parallel::v1

#else

#pragma once

#include <hpx/config.hpp>
#include <hpx/datastructures/tuple.hpp>

#include <hpx/parallel/algorithms/sort.hpp>
#include <hpx/parallel/util/zip_iterator.hpp>

#include <algorithm>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx { namespace parallel { inline namespace v1 {

    template <typename KeyIter, typename ValueIter>
    using sort_by_key_result = std::pair<KeyIter, ValueIter>;

    ///////////////////////////////////////////////////////////////////////////
    // sort
    namespace detail {
        /// \cond NOINTERNAL
        struct extract_key
        {
            template <typename Tuple>
            auto operator()(Tuple&& t) const
                -> decltype(hpx::get<0>(HPX_FORWARD(Tuple, t)))
            {
                return hpx::get<0>(HPX_FORWARD(Tuple, t));
            }
        };
        /// \endcond
    }    // namespace detail

    template <typename ExPolicy, typename KeyIter, typename ValueIter,
        typename Compare = detail::less>
    util::detail::algorithm_result_t<ExPolicy,
        sort_by_key_result<KeyIter, ValueIter>>
    sort_by_key(ExPolicy&& policy, KeyIter key_first, KeyIter key_last,
        ValueIter value_first, Compare&& comp = Compare())
    {
#if !defined(HPX_HAVE_TUPLE_RVALUE_SWAP)
        static_assert(sizeof(KeyIter) == 0,    // always false
            "sort_by_key is not supported unless HPX_HAVE_TUPLE_RVALUE_SWAP "
            "is defined");
#else
        static_assert((hpx::traits::is_random_access_iterator_v<KeyIter>),
            "Requires a random access iterator.");
        static_assert((hpx::traits::is_random_access_iterator_v<ValueIter>),
            "Requires a random access iterator.");

        ValueIter value_last = value_first;
        std::advance(value_last, std::distance(key_first, key_last));

        using iterator_type = hpx::util::zip_iterator<KeyIter, ValueIter>;

        return detail::get_iter_pair<iterator_type>(
            detail::sort<iterator_type>().call(HPX_FORWARD(ExPolicy, policy),
                hpx::util::zip_iterator(key_first, value_first),
                hpx::util::zip_iterator(key_last, value_last),
                HPX_FORWARD(Compare, comp), detail::extract_key()));
#endif
    }
}}}    // namespace hpx::parallel::v1

#endif
