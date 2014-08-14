/*
** Copyright (C) 2014 Cisco and/or its affiliates. All rights reserved.
 * Copyright (C) 2002-2013 Sourcefire, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 2 as
 * published by the Free Software Foundation.  You may not use, modify or
 * distribute this program under any other version of the GNU General
 * Public License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
// config_checksums.cc author Josh Rosenbaum <jrosenba@cisco.com>

#include <sstream>
#include <vector>

#include "conversion_state.h"
#include "utils/converter.h"
#include "utils/s2l_util.h"

namespace config
{

namespace {

class ConfigChecksum : public ConversionState
{
public:
    ConfigChecksum( Converter* cv, LuaData* ld,
                        const std::string* snort_option,
                        const std::string* lua_table,
                        const std::string* lua_option) :
            ConversionState(cv, ld),
            snort_option(snort_option),
            lua_table(lua_table),
            lua_option(lua_option)
    {
    };

    virtual ~ConfigChecksum() {};
    virtual bool convert(std::istringstream& stream)
    {
        std::string val;
        bool retval = true;

        if (snort_option == nullptr || lua_table == nullptr)
            return false;


        ld->open_table(*lua_table);


        if(lua_option == nullptr)
        {
            lua_option = snort_option;
        }
        else if (snort_option->compare(*lua_option))
        {
            ld->add_diff_option_comment(*snort_option, *lua_option);
        }

        while (stream >> val)
            retval = ld->add_list_to_table(*lua_option, val) && retval;

        return retval;
    }

private:
    const std::string* snort_option;
    const std::string* lua_table;
    const std::string* lua_option;
};


template<const std::string *snort_option,
         const std::string *lua_name,
         const std::string *lua_option = nullptr>
static ConversionState* config_checksum_ctor(Converter* cv, LuaData* ld)
{
    return new ConfigChecksum(cv, ld, snort_option, lua_name, lua_option);
}



} // namespace

/**************************
 *******  A P I ***********
 **************************/


static const std::string network = "network";
static const std::string checksum_mode = "checksum_mode";
static const std::string checksum_eval = "checksum_eval";
static const std::string checksum_drop = "checksum_drop";

static const ConvertMap config_checksum_mode =
{
    checksum_mode,
    config_checksum_ctor<&checksum_mode, &network, &checksum_eval>,
};

static const ConvertMap config_checksum_drop =
{
    checksum_drop,
    config_checksum_ctor<&checksum_drop, &network>,
};

const ConvertMap* checksum_mode_map = &config_checksum_mode;
const ConvertMap* checksum_drop_map = &config_checksum_drop;

} // namespace config
