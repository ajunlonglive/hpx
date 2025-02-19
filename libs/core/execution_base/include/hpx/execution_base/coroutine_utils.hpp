//  Copyright (c) 2022 Shreyas Atre
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_CXX20_COROUTINES)

#include <hpx/assert.hpp>
#include <hpx/datastructures/variant.hpp>
#include <hpx/execution_base/completion_signatures.hpp>
#include <hpx/execution_base/get_env.hpp>
#include <hpx/execution_base/traits/coroutine_traits.hpp>
#include <hpx/functional/detail/tag_fallback_invoke.hpp>
#include <hpx/type_support/meta.hpp>

#include <exception>
#include <system_error>
#include <utility>

namespace hpx::execution::experimental {

    // execution::as_awaitable [execution.coro_utils.as_awaitable]
    // as_awaitable is used to transform an object into one that is
    // awaitable within a particular coroutine.
    //
    // as_awaitable is a customization point object.
    // For some subexpressions e and p where p is an lvalue,
    // E names the type decltype((e)) and P names the type decltype((p)),
    // as_awaitable(e, p) is expression-equivalent to the following:
    //
    //      1. tag_invoke(as_awaitable, e, p)
    //         if that expression is well-formed.
    //         -- Mandates: is-awaitable<A> is true,
    //          where A is the type of the tag_invoke expression above.
    //      2. Otherwise, e if is-awaitable<E> is true.
    //      3. Otherwise, sender-awaitable{e, p} if awaitable-sender<E, P>
    //      is true.
    //      4. Otherwise, e.
    struct as_awaitable_t;

    struct connect_awaitable_t;

    namespace detail {
        template <typename Value>
        struct receiver_base;

        template <typename PromiseId, typename Value>
        struct sender_awaitable_base;

        template <typename PromiseId, typename SenderId>
        struct sender_awaitable;

    }    // namespace detail

    namespace detail {
        struct with_awaitable_senders_base;

    }    // namespace detail

    // with_awaitable_senders, when used as the base class of a coroutine
    // promise type, makes senders awaitable in that coroutine type. In
    // addition, it provides a default implementation of unhandled_stopped()
    // such that if a sender completes by calling execution::set_stopped, it
    // is treated as if an uncatchable "stopped" exception were thrown from
    // the await-expression. In practice, the coroutine is never resumed, and
    // the unhandled_stopped of the coroutine caller's promise type is called.
    //
    template <typename Promise>
    struct with_awaitable_senders;

    struct promise_base;

    struct operation_base;

    template <typename ReceiverId>
    struct promise;

    template <typename ReceiverId>
    struct operation;

}    // namespace hpx::execution::experimental

#endif    // HPX_HAVE_CXX20_COROUTINES
