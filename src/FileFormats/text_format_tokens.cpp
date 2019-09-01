/*
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017-2019 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "text_format_tokens.h"
#include "metaparameter.h"
#include <cstdio>
#include <cstring>
#include <cctype>

namespace TextFormatTokens {

///
bool Whitespace::isValid() const
{
    for(const char *p = m_text; *p != '\0'; ++p)
    {
        if(!std::isspace((unsigned char)*p))
            return false;;
    }
    return true;
}

///
Int::Int(int defaultValue)
    : m_defaultValue(defaultValue)
{
}

const char *Int::text() const
{
    std::sprintf(m_buf, "%d", m_defaultValue);
    return m_buf;
}

///
Val::Val(const char *id, unsigned flags)
{
    for(const MetaParameter &mp : MP_instrument)
    {
        if((mp.flags & flags) == flags && !std::strcmp(id, mp.name))
        {
            m_param = &mp;
            break;
        }
    }
}

const char *Val::text() const
{
    if(!isValid())
        return "";

    return m_param->name;
}

bool Val::isValid() const
{
    return m_param != nullptr;
}

} // namespace TextFormatTokens