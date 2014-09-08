/****************************************************************************
 *
** Copyright (C) 2014 Cisco and/or its affiliates. All rights reserved.
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
//  @brief      NHttp Inspector class.
//


#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdexcept>

#include "snort.h"
#include "stream/stream_api.h"
#include "nhttp_enum.h"
#include "nhttp_stream_splitter.h"
#include "nhttp_api.h"
#include "nhttp_inspect.h"

using namespace NHttpEnums;

NHttpInspect::NHttpInspect(bool test_input_, bool test_output_) : test_output(test_output_)
{
    NHttpTestInput::test_input = test_input_;
    if (NHttpTestInput::test_input) {
        NHttpTestInput::test_input_source = new NHttpTestInput(test_input_file);
    }
}

NHttpInspect::~NHttpInspect ()
{
    if (NHttpTestInput::test_input) {
        delete NHttpTestInput::test_input_source;
        if (test_out) {
            fclose(test_out);
        }
    }
}

bool NHttpInspect::enabled ()
{
    return true;
}

bool NHttpInspect::configure (SnortConfig *)
{
    return true;
}

bool NHttpInspect::get_buf(
    InspectionBuffer::Type ibt, Packet* p, InspectionBuffer& b)
{
    switch ( ibt )
    {
    case InspectionBuffer::IBT_KEY:
        return get_buf(HTTP_BUFFER_URI, p, b);

    case InspectionBuffer::IBT_HEADER:
        return get_buf(HTTP_BUFFER_HEADER, p, b);

    case InspectionBuffer::IBT_BODY:
        return get_buf(HTTP_BUFFER_CLIENT_BODY, p, b);

    default:
        return false;
    }   
}

bool NHttpInspect::get_buf(unsigned id, Packet*, InspectionBuffer& b)
{
    const HttpBuffer* h = GetHttpBuffer((HTTP_BUFFER)id);

    if ( !h )
        return false;

    b.data = h->buf;
    b.len = h->length;
    return true;
}

int NHttpInspect::verify(SnortConfig*)
{
    return 0;
}

void NHttpInspect::show(SnortConfig*)
{
    LogMessage("NHttpInspect\n");
}

ProcessResult NHttpInspect::process(const uint8_t* data, const uint16_t dsize, Flow* const flow, SourceId source_id, bool buf_owner)
{
    NHttpFlowData* session_data = (NHttpFlowData*)flow->get_application_data(NHttpFlowData::nhttp_flow_id);
    assert(session_data);

    NHttpMsgSection *msg_section = nullptr;

    switch (session_data->section_type[source_id]) {
      case SEC_REQUEST: msg_section = new NHttpMsgRequest(data, dsize, session_data, source_id, buf_owner); break;
      case SEC_STATUS: msg_section = new NHttpMsgStatus(data, dsize, session_data, source_id, buf_owner); break;
      case SEC_HEADER: msg_section = new NHttpMsgHeader(data, dsize, session_data, source_id, buf_owner); break;
      case SEC_BODY: msg_section = new NHttpMsgBody(data, dsize, session_data, source_id, buf_owner); break;
      case SEC_CHUNKHEAD: msg_section = new NHttpMsgChunkHead(data, dsize, session_data, source_id, buf_owner); break;
      case SEC_CHUNKBODY: msg_section = new NHttpMsgChunkBody(data, dsize, session_data, source_id, buf_owner); break;
      case SEC_TRAILER: msg_section = new NHttpMsgTrailer(data, dsize, session_data, source_id, buf_owner); break;
      case SEC_DISCARD: if (buf_owner) delete[] data; return RES_IGNORE;
      default: assert(0); if (buf_owner) delete[] data; return RES_IGNORE;
    }

    msg_section->analyze();
    msg_section->update_flow();
    msg_section->gen_events();

    ProcessResult return_value = msg_section->worth_detection();
    if (return_value == RES_INSPECT) {
        msg_section->legacy_clients();
    }

    if (test_output) {
        if (!NHttpTestInput::test_input) {
            msg_section->print_section(stdout);
        }
        else {
            if (NHttpTestInput::test_input_source->get_test_number() != file_test_number) {
                if (test_out) fclose (test_out);
                file_test_number = NHttpTestInput::test_input_source->get_test_number();
                char file_name[100];
                snprintf(file_name, sizeof(file_name), "%s%" PRIi64 ".txt", test_output_prefix, file_test_number);
                if ((test_out = fopen(file_name, "w+")) == nullptr) throw std::runtime_error("Cannot open test output file");
            }
            msg_section->print_section(test_out);
            printf("Finished processing section from test %" PRIi64 "\n", file_test_number);
        }
        fflush(nullptr);
    }

    return return_value;
}

