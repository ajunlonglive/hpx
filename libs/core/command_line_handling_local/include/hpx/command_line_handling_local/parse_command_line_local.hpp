//  Copyright (c) 2007-2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/ini/ini.hpp>

#include <hpx/modules/program_options.hpp>

#include <cstddef>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace hpx {
    namespace util {
        enum commandline_error_mode
        {
            return_on_error,
            rethrow_on_error,
            allow_unregistered,
            ignore_aliases = 0x40,
            report_missing_config_file = 0x80
        };

        namespace detail {
            ///////////////////////////////////////////////////////////////////
            inline std::string enquote(std::string const& arg)
            {
                if (arg.find_first_of(" \t\"") != std::string::npos)
                    return std::string("\"") + arg + "\"";
                return arg;
            }
        }    // namespace detail
    }        // namespace util

    namespace local { namespace detail {

        HPX_CORE_EXPORT std::string trim_whitespace(std::string const& s);

        struct HPX_CORE_EXPORT option_parser
        {
            option_parser(util::section const& ini, bool ignore_aliases);

            std::pair<std::string, std::string> operator()(
                std::string const& s) const;

            util::section const& ini_;
            bool ignore_aliases_;
        };

        HPX_CORE_EXPORT hpx::program_options::basic_command_line_parser<char>&
        get_commandline_parser(
            hpx::program_options::basic_command_line_parser<char>& p, int mode);

        HPX_CORE_EXPORT std::vector<std::string> read_config_file_options(
            std::string const& filename, int error_mode);

        enum class options_type
        {
            commandline_options,
            hpx_options,
            hidden_options,
            config_options,
            debugging_options,
            counter_options,
            desc_cfgfile,
            desc_cmdline
        };

        using options_map =
            std::map<options_type, hpx::program_options::options_description>;

        HPX_CORE_EXPORT options_map compose_local_options();
        HPX_CORE_EXPORT void compose_all_options(
            hpx::program_options::options_description const& app_options,
            options_map& all_options);

        HPX_CORE_EXPORT std::string reconstruct_command_line(
            hpx::program_options::variables_map const& vm);

        HPX_CORE_EXPORT bool parse_commandline(util::section const& rtcfg,
            options_map& all_options,
            hpx::program_options::options_description const& app_options,
            std::vector<std::string> const& args,
            hpx::program_options::variables_map& vm, int error_mode,
            hpx::program_options::options_description* visible,
            std::vector<std::string>* unregistered_options);

        HPX_CORE_EXPORT bool parse_commandline(hpx::util::section const& rtcfg,
            hpx::program_options::options_description const& app_options,
            std::string const& cmdline, hpx::program_options::variables_map& vm,
            int error_mode = util::return_on_error,
            hpx::program_options::options_description* visible = nullptr,
            std::vector<std::string>* unregistered_options = nullptr);

        HPX_CORE_EXPORT bool parse_commandline(hpx::util::section const& rtcfg,
            hpx::program_options::options_description const& app_options,
            std::string const& arg0, std::vector<std::string> const& args,
            hpx::program_options::variables_map& vm,
            int error_mode = util::return_on_error,
            hpx::program_options::options_description* visible = nullptr,
            std::vector<std::string>* unregistered_options = nullptr);

        HPX_CORE_EXPORT std::string reconstruct_command_line(
            hpx::program_options::variables_map const& vm);
    }}    // namespace local::detail
}    // namespace hpx
