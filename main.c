// Copyright 2017 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "HelloWorld.h"

#include <uxr/client/client.h>
#include <ucdr/microcdr.h>

#include <stdio.h> //printf
#include <string.h> //strcmp
#include <stdlib.h> //atoi

#include "selfcom_transport.h"

#define STREAM_HISTORY  8
#define BUFFER_SIZE     UXR_CONFIG_UDP_TRANSPORT_MTU* STREAM_HISTORY

void on_topic(
        uxrSession* session,
        uxrObjectId object_id,
        uint16_t request_id,
        uxrStreamId stream_id,
        struct ucdrBuffer* ub,
        uint16_t length,
        void* args)
{
    (void) session; (void) object_id; (void) request_id; (void) stream_id; (void) length;

    HelloWorld topic;
    HelloWorld_deserialize_topic(ub, &topic);

    printf("Received topic: %s, id: %i\n", topic.message, topic.index);

    uint32_t* count_ptr = (uint32_t*) args;
    (*count_ptr)++;
}
uint8_t output_reliable_stream_buffer[BUFFER_SIZE];
int main(
        int args,
        char** argv)
{
    uint32_t max_topics = 1;

    // Transport
    self_com_transport_init();
    uxrCommunication *comm = self_com_get_instance_info();
    // State
    uint32_t icount = 0;
    // Session
    uxrSession session;
    uxr_init_session(&session, 0x81, comm, 0);
    uxr_set_topic_callback(&session, on_topic, &icount);
    // Streams
    uxrStreamId reliable_out = uxr_create_output_reliable_stream(&session, output_reliable_stream_buffer, BUFFER_SIZE,
                    STREAM_HISTORY);

    uint8_t input_reliable_stream_buffer[BUFFER_SIZE];
    uxr_create_input_reliable_stream(&session, input_reliable_stream_buffer, BUFFER_SIZE, STREAM_HISTORY);

    // Create entities
    uxrObjectId object_id = uxr_object_id(0x01, UXR_DATAREADER_ID);

    // Write topics
    bool connected = true;
    uint32_t count = 0;
    while (connected && count < max_topics)
    {
        HelloWorld topic = {
            ++count, "Hello DDS world!"
        };

        ucdrBuffer ub;
        uint32_t topic_size = HelloWorld_size_of_topic(&topic, 0);
        uxr_prepare_output_stream(&session, reliable_out, object_id, &ub, topic_size);
        HelloWorld_serialize_topic(&ub, &topic);
        topic.index = 2;
        uxr_prepare_output_stream(&session, reliable_out, object_id, &ub, topic_size);
        HelloWorld_serialize_topic(&ub, &topic);
        printf("Send topic: %s, id: %i\n", topic.message, topic.index);
        connected = uxr_run_session_time(&session, 1000);
    }

    // Delete resources
    // uxr_delete_session(&session);
    // uxr_close_udp_transport(&transport);

    return 0;
}