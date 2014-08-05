/****************************************************************************
 *
 * Copyright (C) 2014 Cisco and/or its affiliates. All rights reserved.
 * Copyright (C) 2003-2013 Sourcefire, Inc.
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
 *
 ****************************************************************************/

//
//  @author     Tom Peters <thopeter@cisco.com>
//
//  @brief      Field class
//

#include <sys/types.h>
#include <stdio.h>

#include "snort.h"
#include "nhttp_enum.h"
#include "nhttp_field.h"

using namespace NHttpEnums;

const Field Field::FIELD_NULL { STAT_NOSOURCE };

void Field::print(FILE *output, const char* name, bool intVals) const {
    if ((length == STAT_NOTPRESENT) || (length == STAT_NOTCOMPUTE) || (length == STAT_NOSOURCE)) return;
    int outCount = fprintf(output, "%s, length = %d, ", name, length);
    if (length <= 0) {
        fprintf(output, "\n");
        return;
    }
    int32_t printLength = (length <= 1000) ? length : 1000;    // Limit the amount of data printed
    for (int k=0; k < printLength; k++) {
        if ((start[k] >= 0x20) && (start[k] <= 0x7E)) fprintf(output, "%c", (char)start[k]);
        else if (start[k] == 0xD) fprintf(output, "~");
        else if (start[k] == 0xA) fprintf(output, "^");
        else fprintf(output, "*");
        if ((k%120 == (119 - outCount)) && (k+1 < printLength)) fprintf(output, "\n");
    }

    if (intVals && (printLength%8 == 0)) {
        fprintf(output, "\nInteger values =");
        for (int j=0; j < printLength; j+=8) {
            fprintf(output, " %" PRIu64 , *((const uint64_t*)(start+j)));
        }
    }
    fprintf(output, "\n");
}


















