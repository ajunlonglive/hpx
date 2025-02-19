//  Copyright (c) 2019-2020 ETH Zurich
//  Copyright (c) 2007-2022 Hartmut Kaiser
//  Copyright (c) 2019 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/allocator_support/internal_allocator.hpp>
#include <hpx/assert.hpp>
#include <hpx/async_base/launch_policy.hpp>
#include <hpx/execution/algorithms/detail/predicates.hpp>
#include <hpx/execution/detail/async_launch_policy_dispatch.hpp>
#include <hpx/execution/detail/future_exec.hpp>
#include <hpx/execution/detail/post_policy_dispatch.hpp>
#include <hpx/execution/detail/sync_launch_policy_dispatch.hpp>
#include <hpx/execution/executors/execution_parameters.hpp>
#include <hpx/execution/executors/fused_bulk_execute.hpp>
#include <hpx/execution/executors/static_chunk_size.hpp>
#include <hpx/execution_base/execution.hpp>
#include <hpx/execution_base/traits/is_executor.hpp>
#include <hpx/executors/detail/hierarchical_spawning.hpp>
#include <hpx/functional/bind_back.hpp>
#include <hpx/functional/deferred_call.hpp>
#include <hpx/functional/invoke.hpp>
#include <hpx/functional/one_shot.hpp>
#include <hpx/futures/future.hpp>
#include <hpx/futures/traits/future_traits.hpp>
#include <hpx/iterator_support/range.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/topology.hpp>
#include <hpx/serialization/serialize.hpp>
#include <hpx/threading_base/annotated_function.hpp>
#include <hpx/threading_base/scheduler_base.hpp>
#include <hpx/threading_base/thread_data.hpp>
#include <hpx/threading_base/thread_helpers.hpp>
#include <hpx/threading_base/thread_pool_base.hpp>
#include <hpx/type_support/unused.hpp>

#include <algorithm>
#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx { namespace parallel { namespace execution { namespace detail {
    template <typename Policy>
    struct get_default_policy
    {
        static constexpr Policy call() noexcept
        {
            return Policy{};
        }
    };

    template <>
    struct get_default_policy<hpx::launch>
    {
        static constexpr hpx::launch::async_policy call() noexcept
        {
            return hpx::launch::async_policy{};
        }
    };

    ///////////////////////////////////////////////////////////////////////
    template <typename F, typename Shape, typename... Ts>
    struct bulk_function_result;

    ///////////////////////////////////////////////////////////////////////
    template <typename F, typename Shape, typename Future, typename... Ts>
    struct bulk_then_execute_result;

    template <typename F, typename Shape, typename Future, typename... Ts>
    struct then_bulk_function_result;
}}}}    // namespace hpx::parallel::execution::detail

namespace hpx { namespace execution {
    ///////////////////////////////////////////////////////////////////////////
    /// A \a parallel_executor creates groups of parallel execution agents
    /// which execute in threads implicitly created by the executor. This
    /// executor prefers continuing with the creating thread first before
    /// executing newly created threads.
    ///
    /// This executor conforms to the concepts of a TwoWayExecutor,
    /// and a BulkTwoWayExecutor
    template <typename Policy>
    struct parallel_policy_executor
    {
        /// Associate the parallel_execution_tag executor tag type as a default
        /// with this executor, except if the given launch policy is synch.
        using execution_category =
            std::conditional_t<std::is_same_v<Policy, launch::sync_policy>,
                sequenced_execution_tag, parallel_execution_tag>;

        /// Associate the static_chunk_size executor parameters type as a default
        /// with this executor.
        using executor_parameters_type = static_chunk_size;

        /// Create a new parallel executor
        constexpr explicit parallel_policy_executor(
            threads::thread_priority priority,
            threads::thread_stacksize stacksize =
                threads::thread_stacksize::default_,
            threads::thread_schedule_hint schedulehint = {},
            Policy l =
                parallel::execution::detail::get_default_policy<Policy>::call(),
            std::size_t hierarchical_threshold =
                hierarchical_threshold_default_)
          : pool_(nullptr)
          , policy_(l, priority, stacksize, schedulehint)
          , hierarchical_threshold_(hierarchical_threshold)
        {
        }

        constexpr explicit parallel_policy_executor(
            threads::thread_stacksize stacksize,
            threads::thread_schedule_hint schedulehint = {},
            Policy l =
                parallel::execution::detail::get_default_policy<Policy>::call())
          : pool_(nullptr)
          , policy_(
                l, threads::thread_priority::default_, stacksize, schedulehint)
        {
        }

        constexpr explicit parallel_policy_executor(
            threads::thread_schedule_hint schedulehint,
            Policy l =
                parallel::execution::detail::get_default_policy<Policy>::call())
          : pool_(nullptr)
          , policy_(l, threads::thread_priority::default_,
                threads::thread_stacksize::default_, schedulehint)
        {
        }

        constexpr explicit parallel_policy_executor(
            Policy l =
                parallel::execution::detail::get_default_policy<Policy>::call())
          : pool_(nullptr)
          , policy_(l)
        {
        }

        constexpr explicit parallel_policy_executor(
            threads::thread_pool_base* pool,
            threads::thread_priority priority =
                threads::thread_priority::default_,
            threads::thread_stacksize stacksize =
                threads::thread_stacksize::default_,
            threads::thread_schedule_hint schedulehint = {},
            Policy l =
                parallel::execution::detail::get_default_policy<Policy>::call(),
            std::size_t hierarchical_threshold =
                hierarchical_threshold_default_)
          : pool_(pool)
          , policy_(l, priority, stacksize, schedulehint)
          , hierarchical_threshold_(hierarchical_threshold)
        {
        }

    private:
        // property implementations

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
        friend constexpr parallel_policy_executor tag_invoke(
            hpx::execution::experimental::with_annotation_t,
            parallel_policy_executor const& exec, char const* annotation)
        {
            auto exec_with_annotation = exec;
            exec_with_annotation.annotation_ = annotation;
            return exec_with_annotation;
        }

        friend parallel_policy_executor tag_invoke(
            hpx::execution::experimental::with_annotation_t,
            parallel_policy_executor const& exec, std::string annotation)
        {
            auto exec_with_annotation = exec;
            exec_with_annotation.annotation_ =
                hpx::detail::store_function_annotation(HPX_MOVE(annotation));
            return exec_with_annotation;
        }

        friend constexpr char const* tag_invoke(
            hpx::execution::experimental::get_annotation_t,
            parallel_policy_executor const& exec) noexcept
        {
            return exec.annotation_;
        }
#endif

        friend constexpr parallel_policy_executor tag_invoke(
            hpx::parallel::execution::with_processing_units_count_t,
            parallel_policy_executor const& exec, std::size_t num_cores)
        {
            auto exec_with_num_cores = exec;
            exec_with_num_cores.num_cores_ = num_cores;
            return exec_with_num_cores;
        }

        friend constexpr std::size_t tag_invoke(
            hpx::parallel::execution::processing_units_count_t,
            parallel_policy_executor const& exec)
        {
            return exec.get_num_cores();
        }

        friend auto tag_invoke(
            hpx::execution::experimental::get_processing_units_mask_t,
            parallel_policy_executor const& exec)
        {
            auto pool = exec.pool_ ?
                exec.pool_ :
                threads::detail::get_self_or_default_pool();
            return pool->get_used_processing_units(exec.get_num_cores(), false);
        }

        friend auto tag_invoke(hpx::execution::experimental::get_cores_mask_t,
            parallel_policy_executor const& exec)
        {
            auto pool = exec.pool_ ?
                exec.pool_ :
                threads::detail::get_self_or_default_pool();
            return pool->get_used_processing_units(exec.get_num_cores(), true);
        }

    public:
        // backwards compatibility support, will be removed in the future
        template <typename Parameters>
        std::size_t processing_units_count(Parameters&&) const
        {
            return get_num_cores();
        }

    public:
        /// \cond NOINTERNAL
        constexpr bool operator==(
            parallel_policy_executor const& rhs) const noexcept
        {
            return policy_ == rhs.policy_ && pool_ == rhs.pool_ &&
                hierarchical_threshold_ == rhs.hierarchical_threshold_;
        }

        constexpr bool operator!=(
            parallel_policy_executor const& rhs) const noexcept
        {
            return !(*this == rhs);
        }

        constexpr parallel_policy_executor const& context() const noexcept
        {
            return *this;
        }

        void policy(Policy policy) noexcept
        {
            policy_ = HPX_MOVE(policy);
        }

        constexpr Policy const& policy() const noexcept
        {
            return policy_;
        }
        /// \endcond

    private:
        /// \cond NOINTERNAL

        // OneWayExecutor interface
        template <typename F, typename... Ts>
        friend decltype(auto) tag_invoke(
            hpx::parallel::execution::sync_execute_t,
            parallel_policy_executor const& exec, F&& f, Ts&&... ts)
        {
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
            hpx::scoped_annotation annotate(exec.annotation_ ?
                    exec.annotation_ :
                    "parallel_policy_executor::sync_execute");
#endif
            HPX_UNUSED(exec);
            return hpx::detail::sync_launch_policy_dispatch<
                launch::sync_policy>::call(exec.policy_, HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...);
        }

        // TwoWayExecutor interface
        template <typename F, typename... Ts>
        friend decltype(auto) tag_invoke(
            hpx::parallel::execution::async_execute_t,
            parallel_policy_executor const& exec, F&& f, Ts&&... ts)
        {
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
            hpx::util::thread_description desc(f, exec.annotation_);
#else
            hpx::util::thread_description desc(f);
#endif
            auto pool = exec.pool_ ?
                exec.pool_ :
                threads::detail::get_self_or_default_pool();
            return hpx::detail::async_launch_policy_dispatch<Policy>::call(
                exec.policy_, desc, pool, HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename Future, typename... Ts>
        friend decltype(auto) tag_invoke(
            hpx::parallel::execution::then_execute_t,
            parallel_policy_executor const& exec, F&& f, Future&& predecessor,
            Ts&&... ts)
        {
            using result_type =
                hpx::util::detail::invoke_deferred_result_t<F, Future, Ts...>;

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
            auto&& func = hpx::util::one_shot(hpx::bind_back(
                hpx::annotated_function(HPX_FORWARD(F, f), exec.annotation_),
                HPX_FORWARD(Ts, ts)...));
#else
            auto&& func = hpx::util::one_shot(
                hpx::bind_back(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...));
#endif

            hpx::traits::detail::shared_state_ptr_t<result_type> p =
                lcos::detail::make_continuation_alloc_nounwrap<result_type>(
                    hpx::util::internal_allocator<>{},
                    HPX_FORWARD(Future, predecessor), exec.policy_,
                    HPX_MOVE(func));

            return hpx::traits::future_access<hpx::future<result_type>>::create(
                HPX_MOVE(p));
        }

        // NonBlockingOneWayExecutor (adapted) interface
        template <typename F, typename... Ts>
        void post_impl(F&& f, Ts&&... ts) const
        {
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
            hpx::util::thread_description desc(f, annotation_);
#else
            hpx::util::thread_description desc(f);
#endif
            auto pool =
                pool_ ? pool_ : threads::detail::get_self_or_default_pool();
            hpx::detail::post_policy_dispatch<Policy>::call(
                policy_, desc, pool, HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename... Ts>
        friend void tag_invoke(hpx::parallel::execution::post_t,
            parallel_policy_executor const& exec, F&& f, Ts&&... ts)
        {
            exec.post_impl(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        // BulkTwoWayExecutor interface
        // clang-format off
        template <typename F, typename S, typename... Ts,
            HPX_CONCEPT_REQUIRES_(
                !std::is_integral_v<S>
            )>
        // clang-format on
        friend decltype(auto) tag_invoke(
            hpx::parallel::execution::bulk_async_execute_t,
            parallel_policy_executor const& exec, F&& f, S const& shape,
            Ts&&... ts)
        {
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
            hpx::util::thread_description desc(f, exec.annotation_);
#else
            hpx::util::thread_description desc(f);
#endif
            auto pool = exec.pool_ ?
                exec.pool_ :
                threads::detail::get_self_or_default_pool();

            return parallel::execution::detail::hierarchical_bulk_async_execute(
                desc, pool, exec.get_first_core(), exec.get_num_cores(),
                exec.hierarchical_threshold_, exec.policy_, HPX_FORWARD(F, f),
                shape, HPX_FORWARD(Ts, ts)...);
        }

        // clang-format off
        template <typename F, typename S, typename Future, typename... Ts,
            HPX_CONCEPT_REQUIRES_(
                !std::is_integral_v<S>
            )>
        // clang-format on
        friend decltype(auto) tag_invoke(
            hpx::parallel::execution::bulk_then_execute_t,
            parallel_policy_executor const& exec, F&& f, S const& shape,
            Future&& predecessor, Ts&&... ts)
        {
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
            return parallel::execution::detail::
                hierarchical_bulk_then_execute_helper(exec, exec.policy_,
                    hpx::annotated_function(
                        HPX_FORWARD(F, f), exec.annotation_),
                    shape, HPX_FORWARD(Future, predecessor),
                    HPX_FORWARD(Ts, ts)...);
#else
            return parallel::execution::detail::
                hierarchical_bulk_then_execute_helper(exec, exec.policy_,
                    HPX_FORWARD(F, f), shape, HPX_FORWARD(Future, predecessor),
                    HPX_FORWARD(Ts, ts)...);
#endif
        }
        /// \endcond

    private:
        /// \cond NOINTERNAL
        std::size_t get_num_cores() const
        {
            if (num_cores_ != 0)
                return num_cores_;

            auto pool =
                pool_ ? pool_ : threads::detail::get_self_or_default_pool();
            return pool->get_os_thread_count();
        }

        std::size_t get_first_core() const
        {
            if (policy_.hint().mode !=
                    hpx::threads::thread_schedule_hint_mode::none &&
                policy_.hint().hint != -1)
            {
                return policy_.hint().hint;
            }
            return 0;
        }

        friend class hpx::serialization::access;

        template <typename Archive>
        void serialize(Archive& ar, const unsigned int /* version */)
        {
            // clang-format off
            ar & policy_ & hierarchical_threshold_ & num_cores_;
            // clang-format on
        }
        /// \endcond

    private:
        /// \cond NOINTERNAL
        static constexpr std::size_t hierarchical_threshold_default_ = 6;

        threads::thread_pool_base* pool_;
        Policy policy_;
        std::size_t hierarchical_threshold_ = hierarchical_threshold_default_;
        std::size_t num_cores_ = 0;
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
        char const* annotation_ = nullptr;
#endif
        /// \endcond
    };

    // support all properties exposed by the embedded policy
    // clang-format off
    template <typename Tag, typename Policy, typename Property,
        HPX_CONCEPT_REQUIRES_(
            hpx::execution::experimental::is_scheduling_property_v<Tag>
        )>
    // clang-format on
    auto tag_invoke(
        Tag tag, parallel_policy_executor<Policy> const& exec, Property&& prop)
        -> decltype(std::declval<parallel_policy_executor<Policy>>().policy(
                        std::declval<Tag>()(
                            std::declval<Policy>(), std::declval<Property>())),
            parallel_policy_executor<Policy>())
    {
        auto exec_with_prop = exec;
        exec_with_prop.policy(tag(exec.policy(), HPX_FORWARD(Property, prop)));
        return exec_with_prop;
    }

    // clang-format off
    template <typename Tag,typename Policy,
        HPX_CONCEPT_REQUIRES_(
            hpx::execution::experimental::is_scheduling_property_v<Tag>
        )>
    // clang-format on
    auto tag_invoke(Tag tag, parallel_policy_executor<Policy> const& exec)
        -> decltype(std::declval<Tag>()(std::declval<Policy>()))
    {
        return tag(exec.policy());
    }

    using parallel_executor = parallel_policy_executor<hpx::launch>;
}}    // namespace hpx::execution

namespace hpx { namespace parallel { namespace execution {
    /// \cond NOINTERNAL
    template <typename Policy>
    struct is_one_way_executor<hpx::execution::parallel_policy_executor<Policy>>
      : std::true_type
    {
    };

    template <typename Policy>
    struct is_never_blocking_one_way_executor<
        hpx::execution::parallel_policy_executor<Policy>> : std::true_type
    {
    };

    template <typename Policy>
    struct is_two_way_executor<hpx::execution::parallel_policy_executor<Policy>>
      : std::true_type
    {
    };

    template <typename Policy>
    struct is_bulk_two_way_executor<
        hpx::execution::parallel_policy_executor<Policy>> : std::true_type
    {
    };
    /// \endcond
}}}    // namespace hpx::parallel::execution
