#ifndef QUEUE_H_
#define QUEUE_H_
#include <stdint.h>
#define VLQUEUE_16_BIT_LEN       (1)


/*    VLQUEUE_ENABLE_USER_PTR:
 * This option allows the user to provide a user pointer argument into the
 * context block which may be required for certain methods of synchronization.
 *    0 = user pointer is not allocated to context block and init function
 *    1 = user pointer is allocated to context block and init function
 * */
#define VLQUEUE_ENABLE_USER_PTR  (0)


/*    VLQueue Critical section user-definable logic:
 * These blocks are meant to defined to allow the user to adapt the module to
 * the specific environment and synchronization mechanism desired. If only a
 * single thread reads from the queue and a single thread writes from the queue
 * and message counter functionality is not enabled, no synchronization is
 * required.
 * */
/* variables created on the stack which are required for sync*/
#define VLQueue_DefineCriticalVars
/* enter exclusive area functionality */
#define VLQueue_EnterCritical
/* exit exclusive area functionality*/
#define VLQueue_ExitCritical



/******************************************************************************
* CONSTANTS AND MACROS
******************************************************************************/
/* IMPORTANT: Update module version when revisions are changed.
 * BCD: 00[major-2][minor-2][patch-2]*/
#define VLQUEUE_MODULE_VERSION_BCD  (0x010001)

#define VLQUEUE_OK                  (0)
#define VLQUEUE_ERR_PARAM           (-1)
#define VLQUEUE_ERR_QUEUE_FULL      (-2)
#define VLQUEUE_ERR_QUEUE_EMTPY     (-3)
#define VLQUEUE_ERR_READ_BUFFER_SZ  (-4)
#define VLQUEUE_ERR_RANGE           (-5)

/******************************************************************************
* STRUCTURES
******************************************************************************/
typedef struct{
   uint8_t *buffer;
#if VLQUEUE_16_BIT_LEN == 1
   uint16_t bufferLength;
   uint16_t writeIndex;
   uint16_t readIndex;
   uint16_t msgCnt;
#else
   uint32_t bufferLength;
   uint32_t writeIndex;
   uint32_t readIndex;
   uint32_t msgCnt;
#endif
#if VLQUEUE_ENABLE_USER_PTR == 1
   void *   usrPtr;
#endif
} VLQueue_ccb_t;

/******************************************************************************
* PUBLIC FUNCTION DEFINITIONS
******************************************************************************/
/******************************************************************************
* FUNCTION:       queue_getversion
* DESCRIPTION:    This function returns the version of the queue module
* INPUT(S):       ccb: context block to be initialized (also output)
* OUTPUT(S):      void
* RETURN VALUE:   return: less than 0, error; else buffer free space
******************************************************************************/
uint32_t queue_getversion(VLQueue_ccb_t * ccb);

/******************************************************************************
* FUNCTION:       VLQueue_Init
* DESCRIPTION:    This function initializes a queue context block
* INPUT(S):       ccb: context block to be initialized (also output)
*                 buffer: byte array to be used as buffer
*                 length: length of the buffer
*                 *usrPtr: user pointer useful for user defined synchronization
* OUTPUT(S):      ccb: context block
* RETURN VALUE:   return: if 0, success, failure otherwise
******************************************************************************/
#if VLQUEUE_ENABLE_USER_PTR == 1
int32_t queue_init(VLQueue_ccb_t * ccb,
                     uint8_t* buffer,
                     uint32_t length,
                     void *usrPtr);
#else
int32_t queue_init(VLQueue_ccb_t * ccb,
                     uint8_t* buffer,
                     uint32_t length);
#endif

/******************************************************************************
* FUNCTION:       queue_empty
* DESCRIPTION:    This function empties the buffer
* INPUT(S):       ccb: context block to be initialized (also output)
* OUTPUT(S):      none
* RETURN VALUE:   return: less than 0, error; else buffer free space
******************************************************************************/
int32_t queue_empty (VLQueue_ccb_t * ccb);

/******************************************************************************
* FUNCTION:       queue_size
* DESCRIPTION:    This function returns the number of queued messages in the
*                 buffer
* INPUT(S):       ccb: context block to be initialized (also output)
* OUTPUT(S):      void
* RETURN VALUE:   return: less than 0, error; else number of messages queued
******************************************************************************/
int32_t queue_size (VLQueue_ccb_t * ccb);

/******************************************************************************
* FUNCTION:       queue_front
* DESCRIPTION:    This function reads the message at the front of the buffer,
*                 without removing it
* INPUT(S):       ccb: context block to be initialized (also output)
*                 bufSize: size of output buffer
* OUTPUT(S):      *buf: output buffer
* RETURN VALUE:   return: less than 0, error; else the size of the message read
******************************************************************************/
int32_t queue_front (VLQueue_ccb_t * ccb,
                       uint8_t *buf,
                       uint32_t bufSize);

/******************************************************************************
* FUNCTION:       queue_back
* DESCRIPTION:    This function reads the message from the back of the buffer,
*                 without removing it
* INPUT(S):       ccb: context block to be initialized (also output)
*                 bufSize: size of output buffer
* OUTPUT(S):      *buf: output buffer
* RETURN VALUE:   return: less than 0, error; else the size of the message read
******************************************************************************/
int32_t queue_back (VLQueue_ccb_t * ccb,
                    uint8_t *buf,
                    uint32_t bufSize);

/******************************************************************************
* FUNCTION:       queue_push
* DESCRIPTION:    This function queues a variable length message at the
*                 back of the circular buffer it free space permits
* INPUT(S):       ccb: context block to be initialized (also output)
*                 *msg: pointer to message
*                 msgSize: message size
* OUTPUT(S):      none
* RETURN VALUE:   return: equal to zero, success; else error
******************************************************************************/
int32_t queue_push (VLQueue_ccb_t * ccb,
                      const uint8_t *msg,
                      uint32_t msgSize);

/******************************************************************************
* FUNCTION:       queue_pop
* DESCRIPTION:    This function reads the message at the front of the buffer,
*                 and removes it
* INPUT(S):       ccb: context block to be initialized (also output)
*                 bufSize: size of output buffer
* OUTPUT(S):      *buf: output buffer
* RETURN VALUE:   return: less than 0, error; else the size of the message read
******************************************************************************/
int32_t queue_pop (VLQueue_ccb_t * ccb,
                    uint8_t *buf,
                    uint32_t bufSize);
int32_t queue_popPtr_noremove (VLQueue_ccb_t * ccb,
                     uint8_t **buf);
/******************************************************************************
* FUNCTION:       queue_pop_noread
* DESCRIPTION:    This function removes the message at the front of the buffer
* INPUT(S):       ccb: context block to be initialized (also output)
*                 bufSize: size of output buffer
* OUTPUT(S):      *buf: output buffer
* RETURN VALUE:   return: less than 0, error; else the size of the message read
******************************************************************************/
int32_t queue_pop_noread (VLQueue_ccb_t * ccb);

/******************************************************************************
* FUNCTION:       VLQueue_fsize
* DESCRIPTION:    This function returns the size of the message in the front
*                 of the queue
* INPUT(S):       ccb: context block to be initialized (also output)
* RETURN VALUE:   return: less than zero, error; else size of message read
******************************************************************************/
int32_t queue_fsize (VLQueue_ccb_t * ccb);

/******************************************************************************
* FUNCTION:       queue_btotal
* DESCRIPTION:    This function returns the total size of the buffer
* INPUT(S):       ccb: context block to be initialized (also output)
* OUTPUT(S):      void
* RETURN VALUE:   return: less than 0, error; else buffer free space
******************************************************************************/
int32_t queue_btotal (VLQueue_ccb_t * ccb);

/******************************************************************************
* FUNCTION:       queue_bfree
* DESCRIPTION:    This function returns the number of free bytes remaining in
*                 the buffer
* INPUT(S):       ccb: context block to be initialized (also output)
* OUTPUT(S):      void
* RETURN VALUE:   return: less than 0, error; else buffer free space
******************************************************************************/
int32_t queue_bfree (VLQueue_ccb_t * ccb);

/******************************************************************************
* FUNCTION:       queue_check_element_exists
* DESCRIPTION:    This function checks if an element is already present
*                 in queue or not. Used to avoid enqueing of duplicate 
*                 element
* INPUT(S):       ccb: context block to be initialized (also output)
*                 *buf: pointer to message
*                 bufSize: message size
* OUTPUT(S):      none
* RETURN VALUE:   return: less than 0, error; else if element is already 
                  present in queue or not
******************************************************************************/
int32_t queue_check_element_exists (VLQueue_ccb_t * ccb, uint8_t *buf, uint32_t bufSize);


#endif