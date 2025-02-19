# Copyright (c) 2019-2022 The STE||AR-Group
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

set(allocator_support_headers
    hpx/allocator_support/aligned_allocator.hpp
    hpx/allocator_support/allocator_deleter.hpp
    hpx/allocator_support/detail/new.hpp
    hpx/allocator_support/internal_allocator.hpp
    hpx/allocator_support/traits/is_allocator.hpp
)

# cmake-format: off
set(allocator_support_compat_headers
    hpx/allocator_support.hpp => hpx/modules/allocator_support.hpp
    hpx/util/allocator_deleter.hpp => hpx/modules/allocator_support.hpp
    hpx/util/internal_allocator.hpp => hpx/modules/allocator_support.hpp
    hpx/compute/detail/new.hpp => hpx/modules/allocator_support.hpp
)
# cmake-format: on

set(allocator_support_sources)

include(HPX_AddModule)
add_hpx_module(
  core allocator_support
  GLOBAL_HEADER_GEN ON
  SOURCES ${allocator_support_sources}
  HEADERS ${allocator_support_headers}
  COMPAT_HEADERS ${allocator_support_compat_headers}
  DEPENDENCIES hpx_dependencies_allocator
  MODULE_DEPENDENCIES hpx_concepts hpx_config hpx_preprocessor
  CMAKE_SUBDIRS examples tests
)
