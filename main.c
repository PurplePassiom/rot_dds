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
#include <pthread.h>
#include <time.h>

#include "selfcom_transport.h"
#include "session_manager.h"

// #define TEST_DDS
#define STREAM_HISTORY  8
#define BUFFER_SIZE     512

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
#if 0
    HelloWorld topic;
    HelloWorld_deserialize_topic(ub, &topic);

    printf("Received topic: %s, id: %i request_id:%d object_id:%d\n", topic.message, topic.index, request_id,object_id.id);
#else
    uint8_t temp[128];
    ucdr_deserialize_sequence_char(ub, temp, 128, (uint32_t *)&length);
    // printf("\nlen:%d data: ",length);
    for (size_t i = 0; i < length; i++)
    {
        printf("%c",temp[i]);
        // printf("%x-%c ",temp[i],temp[i]);
    }
    printf("--------------send end\n");
    // printf("Received topic: %s,\n", temp);
#endif
    
    uint32_t* count_ptr = (uint32_t*) args;
    (*count_ptr)++;
}
uint8_t output_reliable_stream_buffer1[BUFFER_SIZE];
uint8_t output_reliable_stream_buffer2[BUFFER_SIZE];
// uint8_t output_reliable_stream_buffer2[BUFFER_SIZE];
uxrSession session;
uxrObjectId object_id;
uxrStreamId reliable_out1;
uxrStreamId reliable_out2;
uxrStreamId reliable_in;
uint8_t input_reliable_stream_buffer[BUFFER_SIZE];
pthread_mutex_t mutex;

// 线程函数
void* thread_function(void* arg) {
    struct timespec ts;
    ts.tv_sec = 0;          // 秒
    ts.tv_nsec = 500000;   // 纳秒（10ms
    nanosleep(&ts, NULL);
    ts.tv_sec = 0;
    ts.tv_nsec = 1000000;   // 纳秒（10ms
    
    
    printf("Hello from the thread!\n");
    while (1) {
        pthread_mutex_lock(&mutex);
        #ifdef TEST_DDS
        uxr_run_session_time(&session, 1000);
        #else
        sessioin_excution_function();
        #endif
        pthread_mutex_unlock(&mutex);
        // uxr_run_session_time(&session2, 1000);
        if (nanosleep(&ts, NULL) < 0) {
            perror("nanosleep");
            break;
        }
    }
    return NULL;
}

void* thread2_function(void* arg) {
    uint32_t count = 0;
    struct timespec ts;
    ts.tv_sec = 1;          // 秒
    ts.tv_nsec = 500000000;   // 纳秒（0.5秒）

    while (1) {
        #ifdef TEST_DDS
        HelloWorld topic = {
            ++count, "2:Hello DDS world!"
        };
        ucdrBuffer ub;
        uint32_t topic_size = HelloWorld_size_of_topic(&topic, 0);
        pthread_mutex_lock(&mutex);
        // uxr_prepare_output_stream(&session, reliable_out1, object_id, &ub, topic_size);
        // HelloWorld_serialize_topic(&ub, &topic);
        // uxr_prepare_output_stream(&session, reliable_out1, object_id, &ub, topic_size);
        // HelloWorld_serialize_topic(&ub, &topic);
        // uxr_prepare_output_stream(&session, reliable_out2, object_id, &ub, topic_size);
        // HelloWorld_serialize_topic(&ub, &topic);
        // uxr_prepare_output_stream(&session, reliable_out2, object_id, &ub, topic_size);
        // HelloWorld_serialize_topic(&ub, &topic);
        pthread_mutex_unlock(&mutex);
        #else
        session_manager_send(0, 0xf, (uint8_t*)"hello Wei2", strlen("hello Wei")+1);
        #endif
        if (nanosleep(&ts, NULL) < 0) {
            perror("nanosleep");
            break;
        }
    }
    return NULL;
}

void* thread3_function(void* arg) {
    struct timespec ts;
    uint32_t count = 0;
    ts.tv_sec = 1;          // 秒
    ts.tv_nsec = 500000000;   // 纳秒（0.5秒）
    
    while (1) {
        #ifdef TEST_DDS
        HelloWorld topic = {
            ++count, "3:Hello DDS world!"
        };
        ucdrBuffer ub;
        uint32_t topic_size = HelloWorld_size_of_topic(&topic, 0);
        uxr_prepare_output_stream(&session, reliable_out1, object_id, &ub, topic_size);
        // HelloWorld_serialize_topic(&ub, &topic);
        ucdr_serialize_sequence_char(&ub, (const char*)"hello Wei", strlen("hello Wei")+1);
        // (void) ucdr_serialize_uint32_t(&ub, 99);

        // (void) ucdr_serialize_string(&ub, "data test");
        // uxr_prepare_output_stream(&session, reliable_out1, object_id, &ub, topic_size);
        // HelloWorld_serialize_topic(&ub, &topic);
        #else
        session_manager_send(0, 0xf, (uint8_t*)"hello Wei", strlen("hello Wei")+1);
        #endif
        if (nanosleep(&ts, NULL) < 0) {
            perror("nanosleep");
            break;
        }
    }
    return NULL;
}


int main(
        int args,
        char** argv)
{
    #if 1
    uint32_t max_topics = 1;
    pthread_t thread_id, thread_id2, thread_id3;
    // Transport
    self_com_transport_init();
    #ifdef TEST_DDS
    uxrCommunication *comm = self_com_get_instance_info();
    uxr_init_session(&session, 0x81, comm, 0);
    object_id = uxr_object_id(0x0f, UXR_DATAREADER_ID);
    reliable_out1 = uxr_create_output_best_effort_stream(&session, output_reliable_stream_buffer1, BUFFER_SIZE);
    reliable_out2 = uxr_create_output_best_effort_stream(&session, output_reliable_stream_buffer2, BUFFER_SIZE);
    reliable_in = uxr_create_input_best_effort_stream(&session);
    #else
    session_manager_init(0x1,output_reliable_stream_buffer1,BUFFER_SIZE);
    session_set_on_topic(0, on_topic);
    #endif
    // uxr_init_session(&session, 0x82, comm, 0);
    uint32_t icount = 0;
    uxr_set_topic_callback(&session, on_topic, &icount);
    // 创建线程
    pthread_mutex_init(&mutex, NULL);
    int result = pthread_create(&thread_id, NULL, thread_function, NULL);
    result = pthread_create(&thread_id2, NULL, thread2_function, NULL);
    result = pthread_create(&thread_id3, NULL, thread3_function, NULL);
    if (result != 0) {
        perror("Thread creation failed");
        exit(EXIT_FAILURE);
    }
    // State
    
    // Session
    
    
    // Streams
    
    
    
    // Create entities
    
#if 0
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
#endif
    // Delete resources
    // uxr_delete_session(&session);
    // uxr_close_udp_transport(&transport);
    // 等待线程结束
    result = pthread_join(thread_id, NULL);
    result = pthread_join(thread_id2, NULL);
    result = pthread_join(thread_id3, NULL);
    if (result != 0) {
        perror("Thread join failed");
        exit(EXIT_FAILURE);
    }

    printf("Thread has finished execution.\n");
    return 0;
    #else
    uint32_t max_topics = 50;

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
    uxrStreamId reliable_out = uxr_create_output_best_effort_stream(&session, output_reliable_stream_buffer, BUFFER_SIZE);
    // uxrStreamId reliable_out = uxr_create_output_reliable_stream(&session, output_reliable_stream_buffer, BUFFER_SIZE,
    //                 STREAM_HISTORY);

    uint8_t input_reliable_stream_buffer[BUFFER_SIZE];
    uxr_create_input_best_effort_stream(&session);
    // uxr_create_input_reliable_stream(&session, input_reliable_stream_buffer, BUFFER_SIZE, STREAM_HISTORY);

    // Create entities
    uxrObjectId object_id = uxr_object_id(0x01, UXR_DATAREADER_ID);

    // Write topics
    bool connected = true;
    uint32_t count = 0;
    while (count < max_topics)
    {
        HelloWorld topic = {
            ++count, "Hello DDS world!"
        };

        ucdrBuffer ub;
        uint32_t topic_size = HelloWorld_size_of_topic(&topic, 0);
        uxr_prepare_output_stream(&session, reliable_out, object_id, &ub, topic_size);
        HelloWorld_serialize_topic(&ub, &topic);
        topic.index = 2;
        // uxr_prepare_output_stream(&session, reliable_out, object_id, &ub, topic_size);
        // HelloWorld_serialize_topic(&ub, &topic);
        printf("Send topic: %s, id: %i\n", topic.message, topic.index);
        connected = uxr_run_session_time(&session, 1000);
    }

    // Delete resources
    // uxr_delete_session(&session);
    // uxr_close_udp_transport(&transport);

    return 0;
    #endif
}