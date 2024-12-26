# Copyright 2017 Proyectos y Sistemas de Mantenimiento SL (eProsima).
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.10)
if (${CMAKE_VERSION} VERSION_GREATER 3.0)
    cmake_policy(SET CMP0048 NEW)
endif()

project(PublishHelloWorldClient C)

# if(NOT UCLIENT_BUILD_EXAMPLES)
#     find_package(microxrcedds_client REQUIRED)
# endif()

option(UCLIENT_BUILD_MICROCDR "Force local build of Micro CDR." OFF)
option(UCLIENT_SUPERBUILD "Enable superbuild compilation." ON)
option(UCLIENT_BUILD_TESTS "Build tests." OFF)
option(UCLIENT_BUILD_EXAMPLES "Build examples." OFF)
option(UCLIENT_INSTALL_EXAMPLES "Install examples." OFF)
option(UCLIENT_VERBOSE_SERIALIZATION "Build with serialization verbosity." OFF)
option(UCLIENT_VERBOSE_MESSAGE "Build with message verbosity." OFF)
option(UCLIENT_PIC "Control Position Independent Code." ON)
option(UCLIENT_ISOLATED_INSTALL "Install the project and dependencies into separeted folders with version control." OFF)
option(BUILD_SHARED_LIBS "Control shared/static library building." OFF)
option(BUILD_DOCUMENTATION "Use doxygen to create product documentation" OFF)

option(UCLIENT_BUILD_CI_TESTS "Build CI test cases." OFF)
if(UCLIENT_BUILD_CI_TESTS)
    set(UCLIENT_BUILD_TESTS ON)
    set(UCLIENT_BUILD_EXAMPLES ON)
endif()

if((CMAKE_SYSTEM_NAME STREQUAL "") AND (CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux"))
    option(UCLIENT_MEMORY_TESTS "Build memory tests." OFF)
    option(UCLIENT_PERFORMANCE_TESTS "Build performance tests." OFF)
endif()

option(UCLIENT_BIG_ENDIANNESS "Set the machine endianness to big endianness (by default is little endianness)." OFF)
option(UCLIENT_PROFILE_DISCOVERY "Enable discovery profile." OFF)
option(UCLIENT_PROFILE_UDP "Enable UDP transport." ON)
option(UCLIENT_PROFILE_TCP "Enable TCP transport." ON)
option(UCLIENT_PROFILE_CAN "Enable CAN FD transport." OFF)
option(UCLIENT_PROFILE_SERIAL "Enable Serial transport." ON)
option(UCLIENT_PROFILE_STREAM_FRAMING "Enable stream framing protocol." ON)
set(UCLIENT_MAX_OUTPUT_BEST_EFFORT_STREAMS 1 CACHE STRING "Set the maximum number of output best-effort streams for session.")
set(UCLIENT_MAX_OUTPUT_RELIABLE_STREAMS 1 CACHE STRING "Set the maximum number of output reliable streams for session.")
set(UCLIENT_MAX_INPUT_BEST_EFFORT_STREAMS 1 CACHE STRING "Set the maximum number of input best-effort streams for session.")
set(UCLIENT_MAX_INPUT_RELIABLE_STREAMS 1 CACHE STRING "Set the maximum number of input reliable streams for session.")
set(UCLIENT_MAX_SESSION_CONNECTION_ATTEMPTS 10 CACHE STRING "Set the number of connection attemps.")
set(UCLIENT_MIN_SESSION_CONNECTION_INTERVAL 1000 CACHE STRING "Set the connection interval in milliseconds.")
set(UCLIENT_MIN_HEARTBEAT_TIME_INTERVAL 100 CACHE STRING "Set the time interval between heartbeats in milliseconds.")
set(UCLIENT_UDP_TRANSPORT_MTU 512 CACHE STRING "Set the UDP transport MTU.")
set(UCLIENT_TCP_TRANSPORT_MTU 512 CACHE STRING "Set the TCP transport MTU.")
set(UCLIENT_SERIAL_TRANSPORT_MTU 512 CACHE STRING "Set the Serial transport MTU.")

option(UCLIENT_PROFILE_CUSTOM_TRANSPORT "Enable Custom transport." ON)
set(UCLIENT_CUSTOM_TRANSPORT_MTU 512 CACHE STRING "Set the Custom transport MTU.")

option(UCLIENT_PROFILE_MULTITHREAD "Enable multithread support." ON)
option(UCLIENT_PROFILE_SHARED_MEMORY "Enable shared memory transport support." OFF)
option(UCLIENT_PROFILE_MATCHING "Enable QoS matching support." OFF)
set(UCLIENT_SHARED_MEMORY_MAX_ENTITIES 4 CACHE STRING "Max number of entities involved in shared memory.")
set(UCLIENT_SHARED_MEMORY_STATIC_MEM_SIZE 10 CACHE STRING "Max number data buffers stored in shared memory")

option(UCLIENT_HARD_LIVELINESS_CHECK "Enable hard liveliness check." OFF)
set(UCLIENT_HARD_LIVELINESS_CHECK_TIMEOUT 10000 CACHE STRING "Set the hard liveliness check interval in milliseconds.")

# Off-standard features and tweaks
option(UCLIENT_TWEAK_XRCE_WRITE_LIMIT "This feature uses a tweak to allow XRCE WRITE DATA submessages grater than 64 kB." ON)

set(UCLIENT_C_STANDARD 99 CACHE STRING "Version of the C language used to build the library")

set(UCLIENT_PLATFORM_WINDOWS ON)
set(UCLIENT_PLATFORM_POSIX OFF)

# Transport sources.
set(_transport_src)

if(UCLIENT_PROFILE_UDP)
    # list(APPEND _transport_src src/c/profile/transport/ip/udp/udp_transport.c)
    if(UCLIENT_PLATFORM_POSIX)
        # list(APPEND _transport_src src/c/profile/transport/ip/udp/udp_transport_posix.c)
    elseif(UCLIENT_PLATFORM_POSIX_NOPOLL)
        # list(APPEND _transport_src src/c/profile/transport/ip/udp/udp_transport_posix_nopoll.c)
    elseif(UCLIENT_PLATFORM_WINDOWS)
        # list(APPEND _transport_src src/c/profile/transport/ip/udp/udp_transport_windows.c)
    elseif(UCLIENT_PLATFORM_FREERTOS_PLUS_TCP)
        # list(APPEND _transport_src src/c/profile/transport/ip/udp/udp_transport_freertos_plus_tcp.c)
    elseif(UCLIENT_PLATFORM_RTEMS_BSD_NET)
        # list(APPEND _transport_src src/c/profile/transport/ip/udp/udp_transport_rtems_bsd_net.c)
    endif()
endif()

if(UCLIENT_PLATFORM_WINDOWS)
set(UCLIENT_PROFILE_SERIAL OFF)
set(UCLIENT_PROFILE_CAN OFF)
endif()


if(UCLIENT_PROFILE_DISCOVERY OR UCLIENT_PROFILE_UDP OR UCLIENT_PROFILE_TCP)
    if(UCLIENT_PLATFORM_POSIX)
        list(APPEND _transport_src src/c/profile/transport/ip/ip_posix.c)
    elseif(UCLIENT_PLATFORM_POSIX_NOPOLL)
        list(APPEND _transport_src src/c/profile/transport/ip/ip_posix.c)
    elseif(UCLIENT_PLATFORM_WINDOWS)
        list(APPEND _transport_src src/c/profile/transport/ip/ip_windows.c)
    elseif(UCLIENT_PLATFORM_FREERTOS_PLUS_TCP)
        list(APPEND _transport_src src/c/profile/transport/ip/ip_freertos_plus_tcp.c)
    elseif(UCLIENT_PLATFORM_RTEMS_BSD_NET)
        list(APPEND _transport_src src/c/profile/transport/ip/ip_rtems_bsd_net.c)
    endif()
endif()

if(UCLIENT_PROFILE_DISCOVERY)
    list(APPEND _transport_src src/c/profile/discovery/discovery.c)
    list(APPEND _transport_src src/c/profile/transport/ip/udp/udp_transport.c)
    if(UCLIENT_PLATFORM_POSIX)
        # list(APPEND _transport_src src/c/profile/discovery/transport/udp_transport_datagram_posix.c)
    elseif(UCLIENT_PLATFORM_POSIX_NOPOLL)
        # list(APPEND _transport_src src/c/profile/discovery/transport/udp_transport_datagram_posix_nopoll.c)
    elseif(UCLIENT_PLATFORM_WINDOWS)
        list(APPEND _transport_src src/c/profile/discovery/transport/udp_transport_datagram_windows.c)
    elseif(UCLIENT_PLATFORM_FREERTOS_PLUS_TCP)
        # list(APPEND _transport_src src/c/profile/discovery/transport/udp_transport_datagram_freertos_plus_tcp.c)
    elseif(UCLIENT_PLATFORM_RTEMS_BSD_NET)
        # list(APPEND _transport_src src/c/profile/discovery/transport/udp_transport_datagram_rtems_bsd_net.c)
    endif()
endif()

if(UCLIENT_PROFILE_CUSTOM_TRANSPORT)
    # list(APPEND _transport_src src/c/profile/transport/custom/custom_transport.c)
endif()

if(UCLIENT_PROFILE_SHARED_MEMORY)
    set(UCLIENT_PROFILE_MATCHING ON)
endif()

# Other sources
set(SRCS
    CDR/array.c
    CDR/basic.c
    CDR/common.c
    CDR/sequence.c
    CDR/string.c
    src/c/core/session/stream/input_best_effort_stream.c
    src/c/core/session/stream/input_reliable_stream.c
    src/c/core/session/stream/output_best_effort_stream.c
    src/c/core/session/stream/output_reliable_stream.c
    src/c/core/session/stream/stream_storage.c
    src/c/core/session/stream/stream_id.c
    src/c/core/session/stream/seq_num.c
    src/c/core/session/session.c
    src/c/core/session/session_info.c
    src/c/core/session/submessage.c
    src/c/core/session/object_id.c
    src/c/core/serialization/xrce_types.c
    src/c/core/serialization/xrce_header.c
    src/c/core/serialization/xrce_subheader.c
    src/c/util/time.c
    src/c/util/ping.c
    src/c/core/session/common_create_entities.c
    src/c/core/session/create_entities_ref.c
    src/c/core/session/create_entities_xml.c
    src/c/core/session/create_entities_bin.c
    src/c/core/session/read_access.c
    src/c/core/session/write_access.c
    selfcom_transport.c
    queue.c
    # src/c/profile/transport/tp_socket.c
    # src/c/profile/multithread/multithread.c
    $<$<BOOL:${UCLIENT_PROFILE_SHARED_MEMORY}>:src/c/profile/shared_memory/shared_memory.c>
    $<$<BOOL:${UCLIENT_PROFILE_MATCHING}>:src/c/profile/matching/matching.c>
    $<$<BOOL:${UCLIENT_PROFILE_STREAM_FRAMING}>:src/c/profile/transport/stream_framing/stream_framing_protocol.c>
    $<$<OR:$<BOOL:${UCLIENT_VERBOSE_MESSAGE}>,$<BOOL:${UCLIENT_VERBOSE_SERIALIZATION}>>:src/c/core/log/log.c>
    ${_transport_src}
    )
###############################################################################
# Set install directories
###############################################################################
if(UCLIENT_ISOLATED_INSTALL)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}-${PROJECT_VERSION}")
endif()

include(GNUInstallDirs)
set(BIN_INSTALL_DIR     ${CMAKE_INSTALL_BINDIR}     CACHE PATH "Installation directory for binaries")
set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR} CACHE PATH "Installation directory for C headers")
set(LIB_INSTALL_DIR     ${CMAKE_INSTALL_LIBDIR}     CACHE PATH "Installation directory for libraries")
set(DATA_INSTALL_DIR    ${CMAKE_INSTALL_DATADIR}    CACHE PATH "Installation directory for data")
if(WIN32)
    set(LICENSE_INSTALL_DIR . CACHE PATH "Installation directory for licenses")
    set(DOC_DIR "doc")
else()
    set(LICENSE_INSTALL_DIR ${DATA_INSTALL_DIR}/${PROJECT_NAME} CACHE PATH "Installation directory for licenses")
    set(DOC_DIR "${DATA_INSTALL_DIR}/doc")
endif()
add_executable(${PROJECT_NAME} ${SRCS} main.c HelloWorld.c)
target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/CDR>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}>
    PRIVATE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/c>
    )
# Generate config.h
# configure_file(${PROJECT_SOURCE_DIR}/include/uxr/client/config.h.in
#     ${PROJECT_SOURCE_DIR}/include/uxr/client/config.h
#     )
# target_compile_options(${PROJECT_NAME} PRIVATE /wd4996)
set_target_properties(${PROJECT_NAME} PROPERTIES
        C_STANDARD 99
        C_STANDARD_REQUIRED YES
        )
target_link_libraries(${PROJECT_NAME} $<$<C_COMPILER_ID:GNU>:-Wl,--gc-section,--no-export-dynamic>)
