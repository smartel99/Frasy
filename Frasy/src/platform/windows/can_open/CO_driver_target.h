/**
 * Windows socketCAN specific definitions for CANopenNode.
 *
 * @file        CO_driver_target.h
 * @ingroup     CO_socketCAN_driver_target
 * @author      Janez Paternoster
 * @author      Martin Wagner
 * @copyright   2004 - 2020 Janez Paternoster
 * @copyright   2018 - 2020 Neuberger Gebaeudeautomation GmbH
 *
 * This file is part of CANopenNode, an opensource CANopen Stack.
 * Project home page is <https://github.com/CANopenNode/CANopenNode>.
 * For more information on CANopen see <http://www.can-cia.org/>.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef CO_DRIVER_TARGET_H
#define CO_DRIVER_TARGET_H

/* This file contains device and application specific definitions.
 * It is included from CO_driver.h, which contains documentation
 * for common definitions below. */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#ifdef CO_DRIVER_CUSTOM
#    include "CO_driver_custom.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


/* Stack configuration override default values.
 * For more information see file CO_config.h. */
#ifdef CO_SINGLE_THREAD
#    define CO_CONFIG_GLOBAL_FLAG_CALLBACK_PRE 0
#else
#    define CO_CONFIG_GLOBAL_FLAG_CALLBACK_PRE CO_CONFIG_FLAG_CALLBACK_PRE
#endif
#define CO_CONFIG_GLOBAL_FLAG_TIMERNEXT CO_CONFIG_FLAG_TIMERNEXT

#ifndef CO_CONFIG_NMT
#    define CO_CONFIG_NMT                                                                                              \
        (CO_CONFIG_NMT_CALLBACK_CHANGE | CO_CONFIG_NMT_MASTER | CO_CONFIG_GLOBAL_FLAG_CALLBACK_PRE |                   \
         CO_CONFIG_GLOBAL_FLAG_TIMERNEXT)
#endif

#ifndef CO_CONFIG_HB_CONS
#    define CO_CONFIG_HB_CONS                                                                                          \
        (CO_CONFIG_HB_CONS_ENABLE | CO_CONFIG_HB_CONS_CALLBACK_CHANGE | CO_CONFIG_GLOBAL_FLAG_CALLBACK_PRE |           \
         CO_CONFIG_GLOBAL_FLAG_TIMERNEXT | CO_CONFIG_GLOBAL_FLAG_OD_DYNAMIC)
#endif

#ifndef CO_CONFIG_EM
#    define CO_CONFIG_EM                                                                                               \
        (CO_CONFIG_EM_PRODUCER | CO_CONFIG_EM_PROD_CONFIGURABLE | CO_CONFIG_EM_PROD_INHIBIT | CO_CONFIG_EM_HISTORY |   \
         CO_CONFIG_EM_STATUS_BITS | CO_CONFIG_EM_CONSUMER | CO_CONFIG_GLOBAL_FLAG_CALLBACK_PRE |                       \
         CO_CONFIG_GLOBAL_FLAG_TIMERNEXT)
#endif

#ifndef CO_CONFIG_SDO_SRV
#    define CO_CONFIG_SDO_SRV                                                                                          \
        (CO_CONFIG_SDO_SRV_SEGMENTED | CO_CONFIG_SDO_SRV_BLOCK | CO_CONFIG_GLOBAL_FLAG_CALLBACK_PRE |                  \
         CO_CONFIG_GLOBAL_FLAG_TIMERNEXT | CO_CONFIG_GLOBAL_FLAG_OD_DYNAMIC)
#endif

#ifndef CO_CONFIG_SDO_SRV_BUFFER_SIZE
#    define CO_CONFIG_SDO_SRV_BUFFER_SIZE 900
#endif

#ifndef CO_CONFIG_SDO_CLI
#    define CO_CONFIG_SDO_CLI                                                                                          \
        (CO_CONFIG_SDO_CLI_ENABLE | CO_CONFIG_SDO_CLI_SEGMENTED | CO_CONFIG_SDO_CLI_BLOCK | CO_CONFIG_SDO_CLI_LOCAL |  \
         CO_CONFIG_GLOBAL_FLAG_CALLBACK_PRE | CO_CONFIG_GLOBAL_FLAG_TIMERNEXT | CO_CONFIG_GLOBAL_FLAG_OD_DYNAMIC)
#endif

#ifndef CO_CONFIG_TIME
#    define CO_CONFIG_TIME                                                                                             \
        (CO_CONFIG_TIME_ENABLE | CO_CONFIG_TIME_PRODUCER | CO_CONFIG_GLOBAL_FLAG_CALLBACK_PRE |                        \
         CO_CONFIG_GLOBAL_FLAG_OD_DYNAMIC)
#endif

#ifndef CO_CONFIG_LSS
#    define CO_CONFIG_LSS                                                                                              \
        (CO_CONFIG_LSS_SLAVE | CO_CONFIG_LSS_SLAVE_FASTSCAN_DIRECT_RESPOND | CO_CONFIG_LSS_MASTER |                    \
         CO_CONFIG_GLOBAL_FLAG_CALLBACK_PRE)
#endif

#ifndef CO_CONFIG_GTW
#    define CO_CONFIG_GTW                                                                                              \
        (CO_CONFIG_GTW_ASCII | CO_CONFIG_GTW_ASCII_SDO | CO_CONFIG_GTW_ASCII_NMT | CO_CONFIG_GTW_ASCII_LSS |           \
         CO_CONFIG_GTW_ASCII_LOG | CO_CONFIG_GTW_ASCII_ERROR_DESC | CO_CONFIG_GTW_ASCII_PRINT_HELP |                   \
         CO_CONFIG_GTW_ASCII_PRINT_LEDS)
#    define CO_CONFIG_GTW_BLOCK_DL_LOOP  3
#    define CO_CONFIG_GTWA_COMM_BUF_SIZE 2000
#    define CO_CONFIG_GTWA_LOG_BUF_SIZE  10000
#endif

#ifndef CO_CONFIG_CRC16
#    define CO_CONFIG_CRC16 (CO_CONFIG_CRC16_ENABLE)
#endif

#ifndef CO_CONFIG_FIFO
#    define CO_CONFIG_FIFO                                                                                             \
        (CO_CONFIG_FIFO_ENABLE | CO_CONFIG_FIFO_ALT_READ | CO_CONFIG_FIFO_CRC16_CCITT |                                \
         CO_CONFIG_FIFO_ASCII_COMMANDS | CO_CONFIG_FIFO_ASCII_DATATYPES)
#endif


/* Print debug info from some internal parts of the stack */
#if (CO_CONFIG_DEBUG) & CO_CONFIG_DEBUG_COMMON
#    include <stdio.h>
#    include <syslog.h>
#    define CO_DEBUG_COMMON(msg) log_printf(LOG_DEBUG, DBG_CO_DEBUG, msg);
#endif


/**
 * @defgroup CO_socketCAN_driver_target CO_driver_target.h
 * Windows socketCAN specific @ref CO_driver definitions for CANopenNode.
 *
 * @ingroup CO_socketCAN
 * @{
 */

/**
 * Multi interface support
 *
 * Enable this to use interface combining at driver level. This
 * adds functions to broadcast/selective transmit messages on the
 * given interfaces as well as combining all received message into
 * one queue.
 *
 * If CO_DRIVER_MULTI_INTERFACE is set to 0, then CO_CANmodule_init()
 * adds single socketCAN interface specified by CANptr argument. In case of
 * failure, CO_CANmodule_init() returns CO_ERROR_SYSCALL.
 *
 * If CO_DRIVER_MULTI_INTERFACE is set to 1, then CO_CANmodule_init()
 * ignores CANptr argument. Interfaces must be added by
 * CO_CANmodule_addInterface() function after CO_CANmodule_init().
 *
 * Macro is set to 0 (disabled) by default. It can be overridden.
 *
 * This is not intended to realize interface redundancy!!!
 */
#ifndef CO_DRIVER_MULTI_INTERFACE
#    define CO_DRIVER_MULTI_INTERFACE 0
#endif

/**
 * CAN bus error reporting
 *
 * CO_DRIVER_ERROR_REPORTING enabled adds support for socketCAN error detection
 * and handling functions inside the driver. This is needed when you have
 * CANopen with "0" connected nodes as a use case, as this is normally
 * forbidden in CAN.
 *
 * Macro is set to 1 (enabled) by default. It can be overridden.
 *
 * you need to enable error reporting in your kernel driver using:
 * @code{.sh}
 * ip link set canX type can berr-reporting on
 * @endcode
 * Of course, the kernel driver for your hardware needs this functionality to be
 * implemented...
 */
#ifndef CO_DRIVER_ERROR_REPORTING
#    define CO_DRIVER_ERROR_REPORTING 1
#endif

/* skip this section for Doxygen, because it is documented in CO_driver.h */
#ifndef CO_DOXYGEN

/* Basic definitions */
#    define CO_LITTLE_ENDIAN
#    define CO_SWAP_16(x) x
#    define CO_SWAP_32(x) x
#    define CO_SWAP_64(x) x
/* NULL is defined in stddef.h */
/* true and false are defined in stdbool.h */
/* int8_t to uint64_t are defined in stdint.h */
typedef bool   bool_t;
typedef float  float32_t;
typedef double float64_t;

/* CAN receive message structure. */
typedef struct {
    uint32_t ident;
    uint8_t  DLC;
    uint8_t  data[8];
} CO_CANrxMsg_t;

/* Access to received CAN message */
static inline uint16_t CO_CANrxMsg_readIdent(void* rxMsg)
{
    CO_CANrxMsg_t* rxMsgCasted = (CO_CANrxMsg_t*)rxMsg;
    return (uint16_t)(rxMsgCasted->ident & 0x1FF);
}
static inline uint8_t CO_CANrxMsg_readDLC(void* rxMsg)
{
    CO_CANrxMsg_t* rxMsgCasted = (CO_CANrxMsg_t*)rxMsg;
    return (uint8_t)(rxMsgCasted->DLC);
}
static inline uint8_t* CO_CANrxMsg_readData(void* rxMsg)
{
    CO_CANrxMsg_t* rxMsgCasted = (CO_CANrxMsg_t*)rxMsg;
    return (uint8_t*)(rxMsgCasted->data);
}

/* Received message object */
typedef struct {
    uint32_t ident;
    uint32_t mask;
    void*    object;
    void     (*CANrx_callback)(void* object, void* message);
} CO_CANrx_t;

/* Transmit message object. */
typedef struct {
    uint32_t        ident;
    uint8_t         DLC;
    uint8_t         data[8];
    volatile bool_t bufferFull;
    volatile bool_t syncFlag; /* info about transmit message */
} CO_CANtx_t;


/* Max COB ID for standard frame format */
#    define CO_CAN_MSG_SFF_MAX_COB_ID (1 << CAN_SFF_ID_BITS)

/* CAN module object */
typedef struct {
    void*             interface;
    CO_CANrx_t*       rxArray;
    uint16_t          rxSize;
    uint32_t          rxDropCount; /* messages dropped on rx socket queue */
    CO_CANtx_t*       txArray;
    uint16_t          txSize;
    uint16_t          CANerrorStatus;
    volatile bool_t   CANnormal;
    volatile uint16_t CANtxCount;
} CO_CANmodule_t;


/* Data storage: Maximum file name length including path */
#    ifndef CO_STORAGE_PATH_MAX
#        define CO_STORAGE_PATH_MAX 255
#    endif

/* Data storage object for one entry */
typedef struct {
    void*   addr;
    size_t  len;
    uint8_t subIndexOD;
    uint8_t attr;
    /* Name of the file, where data block is stored */
    char filename[CO_STORAGE_PATH_MAX];
    /* CRC checksum of the data stored previously, for auto storage */
    uint16_t crc;
    /* Pointer to opened file, for auto storage */
    FILE* fp;
} CO_storage_entry_t;


#    ifdef CO_SINGLE_THREAD
#        define CO_LOCK_CAN_SEND(CAN_MODULE)                                                                           \
            {                                                                                                          \
                (void)CAN_MODULE;                                                                                      \
            }
#        define CO_UNLOCK_CAN_SEND(CAN_MODULE)                                                                         \
            {                                                                                                          \
                (void)CAN_MODULE;                                                                                      \
            }
#        define CO_LOCK_EMCY(CAN_MODULE)                                                                               \
            {                                                                                                          \
                (void)CAN_MODULE;                                                                                      \
            }
#        define CO_UNLOCK_EMCY(CAN_MODULE)                                                                             \
            {                                                                                                          \
                (void)CAN_MODULE;                                                                                      \
            }
#        define CO_LOCK_OD(CAN_MODULE)                                                                                 \
            {                                                                                                          \
                (void)CAN_MODULE;                                                                                      \
            }
#        define CO_UNLOCK_OD(CAN_MODULE)                                                                               \
            {                                                                                                          \
                (void)CAN_MODULE;                                                                                      \
            }
#        define CO_MemoryBarrier()
#    else

/* (un)lock critical section in CO_CANsend() - unused */
#        define CO_LOCK_CAN_SEND(CAN_MODULE)
#        define CO_UNLOCK_CAN_SEND(CAN_MODULE)

/* (un)lock critical section in CO_errorReport() or CO_errorReset() */
int  CO_LOCK_EMCY(CO_CANmodule_t* CANmodule);
void CO_UNLOCK_EMCY(CO_CANmodule_t* CANmodule);

/* (un)lock critical section when accessing Object Dictionary */
int  CO_LOCK_OD(CO_CANmodule_t* CANmodule);
void CO_UNLOCK_OD(CO_CANmodule_t* CANmodule);

/* Synchronization between CAN receive and message processing threads. */
#        define CO_MemoryBarrier()                                                                                     \
            {                                                                                                          \
                __sync_synchronize();                                                                                  \
            }
#    endif /* CO_SINGLE_THREAD */

#    define CO_FLAG_READ(rxNew) ((rxNew) != NULL)
#    define CO_FLAG_SET(rxNew)                                                                                         \
        {                                                                                                              \
            CO_MemoryBarrier();                                                                                        \
            rxNew = (void*)1L;                                                                                         \
        }
#    define CO_FLAG_CLEAR(rxNew)                                                                                       \
        {                                                                                                              \
            CO_MemoryBarrier();                                                                                        \
            rxNew = NULL;                                                                                              \
        }

#endif /* #ifndef CO_DOXYGEN */

/**
 * Receives any pending CAN messages.
 *
 * If there are no messages in the queue, immediately returns.
 *
 * @note This function must be called periodically.
 *
 * @param canModule Pointer to the CANopen instance.
 */
void CO_CANpollReceive(CO_CANmodule_t* canModule);

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CO_DRIVER_TARGET_H */
