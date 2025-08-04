#include "xrce_header_internal.h"

//==================================================================
//                             PUBLIC
//==================================================================
void uxr_serialize_message_header(
        ucdrBuffer* ub,
        uint8_t session_id,
        uint8_t stream_id,
        uint16_t seq_num,
        const uint8_t* key)
{
    (void) stream_id;
    (void) seq_num;
    (void) key;
    (void) ucdr_serialize_uint8_t(ub, session_id);
}

void uxr_deserialize_message_header(
        ucdrBuffer* ub,
        uint8_t* session_id,
        uint8_t* stream_id,
        uint16_t* seq_num,
        uint8_t* key)
{
    (void) key;
    (void) seq_num;
    (void) ucdr_deserialize_uint8_t(ub, session_id);
    *stream_id = 1;
    #if 0
    (void) ucdr_deserialize_uint8_t(ub, stream_id);
    (void) ucdr_deserialize_endian_uint16_t(ub, UCDR_LITTLE_ENDIANNESS, seq_num);
    if (SESSION_ID_WITHOUT_CLIENT_KEY > *session_id)
    {
        (void) ucdr_deserialize_array_uint8_t(ub, key, CLIENT_KEY_SIZE);
    }
    #endif
}
