//  Copyright (c) 2015 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/local/init.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <iterator>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace test {

    template <typename Iterator>
    HPX_FORCEINLINE Iterator previous(Iterator it)
    {
        return --it;
    }

    template <typename Iterator>
    HPX_FORCEINLINE Iterator next(Iterator it)
    {
        return ++it;
    }

    template <typename IteratorBase, typename IteratorValue>
    struct previous_transformer
    {
        previous_transformer() {}

        // at position 'begin' it will dereference 'value', otherwise 'it-1'
        previous_transformer(
            IteratorBase const& begin, IteratorValue const& value)
          : begin_(begin)
          , value_(value)
        {
        }

        template <typename Iterator>
        typename std::iterator_traits<Iterator>::reference operator()(
            Iterator const& it) const
        {
            if (it == begin_)
                return *value_;
            return *test::previous(it);
        }

    private:
        IteratorBase begin_;
        IteratorValue value_;
    };

    template <typename IteratorBase, typename IteratorValue>
    inline previous_transformer<IteratorBase, IteratorValue>
    make_previous_transformer(
        IteratorBase const& base, IteratorValue const& value)
    {
        return previous_transformer<IteratorBase, IteratorValue>(base, value);
    }

    ///////////////////////////////////////////////////////////////////////
    template <typename IteratorBase, typename IteratorValue>
    struct next_transformer
    {
        next_transformer() {}

        // at position 'end' it will dereference 'value', otherwise 'it+1'
        next_transformer(IteratorBase const& end, IteratorValue const& value)
          : end_(end)
          , value_(value)
        {
        }

        template <typename Iterator>
        typename std::iterator_traits<Iterator>::reference operator()(
            Iterator const& it) const
        {
            if (it == end_)
                return *value_;
            return *test::next(it);
        }

    private:
        IteratorBase end_;
        IteratorValue value_;
    };

    template <typename IteratorBase, typename IteratorValue>
    inline next_transformer<IteratorBase, IteratorValue> make_next_transformer(
        IteratorBase const& base, IteratorValue const& value)
    {
        return next_transformer<IteratorBase, IteratorValue>(base, value);
    }
}    // namespace test

///////////////////////////////////////////////////////////////////////////////
// dereference element to the left of current
void test_left_element_full()
{
    // demonstrate use of 'previous' and 'next' transformers
    std::vector<int> values(10);
    std::iota(std::begin(values), std::end(values), 0);

    auto transformer =
        test::make_previous_transformer(std::begin(values), &values.back());

    std::ostringstream str;

    std::for_each(
        hpx::util::transform_iterator(std::begin(values), transformer),
        hpx::util::transform_iterator(std::end(values), transformer),
        [&str](int d) { str << d << " "; });

    HPX_TEST_EQ(str.str(), std::string("9 0 1 2 3 4 5 6 7 8 "));
}

// dereference element to the right of current
void test_right_element_full()
{
    // demonstrate use of 'previous' and 'next' transformers
    std::vector<int> values(10);
    std::iota(std::begin(values), std::end(values), 0);

    auto transformer =
        test::make_next_transformer(std::end(values) - 1, &values.front());

    std::ostringstream str;

    std::for_each(
        hpx::util::transform_iterator(std::begin(values), transformer),
        hpx::util::transform_iterator(std::end(values), transformer),
        [&str](int d) { str << d << " "; });

    HPX_TEST_EQ(str.str(), std::string("1 2 3 4 5 6 7 8 9 0 "));
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    test_left_element_full();
    test_right_element_full();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
