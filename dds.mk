DDSSRC =		dds/srcs/matching/matching.c \
				dds/lqueue.c \
				dds/srcs/serialization/xrce_header.c \
				dds/srcs/serialization/xrce_subheader.c \
				dds/srcs/serialization/xrce_types.c \
				dds/srcs/session/object_id.c \
				dds/srcs/session/read_access.c \
				dds/srcs/session/session_info.c \
				dds/srcs/session/session.c \
				dds/srcs/session/submessage.c \
				dds/srcs/session/write_access.c \
				dds/srcs/util/ping.c \
				dds/selfcom_transport.c \
				dds/session_manager_config.c \
				dds/session_manager.c \
				dds/srcs/util/time.c \
				dds/srcs/session/stream/stream_storage.c \
				dds/srcs/session/stream/stream_id.c \
				dds/srcs/session/stream/input_best_effort_stream.c \
				dds/srcs/session/stream/output_best_effort_stream.c \
				dds/srcs/session/stream/input_reliable_stream.c \
				dds/srcs/session/stream/output_reliable_stream.c \
				dds/srcs/session/stream/seq_num.c \
				dds/CDR/array.c \
				dds/CDR/basic.c \
				dds/CDR/common.c \
				dds/CDR/sequence.c \
				dds/CDR/string.c \
				dds/srcs/multithread/multithread.c

DDSINC =		dds/inc \
				dds/inc/communication \
				dds/inc/profile \
				dds/inc/session \
				dds/inc/type \
				dds/inc/ucdr \
				dds/inc/util \
				dds/inc/session/stream \
				dds/srcs/util \
				dds/srcs/session \
				dds/srcs/session/stream \
				dds/srcs/serialization \
				dds/srcs/matching \
				dds \
# 				dds/srcs/session/common_create_entities.c \
# 				dds/srcs/session/create_entities_bin.c \
# 				dds/srcs/session/create_entities_ref.c \
# 				dds/srcs/session/create_entities_xml.c \