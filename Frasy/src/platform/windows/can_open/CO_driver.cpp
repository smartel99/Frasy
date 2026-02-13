/*
 * Linux socketCAN interface for CANopenNode.
 *
 * @file        CO_driver.c
 * @ingroup     CO_driver
 * @author      Janez Paternoster, Martin Wagner
 * @copyright   2004 - 2015 Janez Paternoster, 2017 - 2020 Neuberger Gebaeudeautomation GmbH
 *
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

#include "utils/communication/can_open/can_open.h"
#include "utils/communication/slcan/device.h"

#include "utils/lua/profile_events.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <mutex>

#include "301/CO_driver.h"

#include "utils/communication/can_open/CO_error.h"

extern "C" {

#define CANID_MASK 0x07FF    //!< CAN standard ID mask.
#define FLAG_RTR   0x8000    //!< RTR flag, par of identifier.

#ifndef CO_SINGLE_THREAD
std::mutex CO_SEND_mutex;
std::mutex CO_EMCY_mutex;
std::mutex CO_OD_mutex;

int CO_LOCK_CAN_SEND([[maybe_unused]] CO_CANmodule_t* CANmodule)
{
    FRASY_PROFILE_FUNCTION();
    CO_SEND_mutex.lock();
    return 0;    // Always assume success.
}

void CO_UNLOCK_CAN_SEND([[maybe_unused]] CO_CANmodule_t* CANmodule)
{
    FRASY_PROFILE_FUNCTION();
    CO_SEND_mutex.unlock();
}

int CO_LOCK_EMCY([[maybe_unused]] CO_CANmodule_t* CANmodule)
{
    FRASY_PROFILE_FUNCTION();
    CO_EMCY_mutex.lock();
    return 0;    // Always assume success.
}

void CO_UNLOCK_EMCY([[maybe_unused]] CO_CANmodule_t* CANmodule)
{
    FRASY_PROFILE_FUNCTION();
    CO_EMCY_mutex.unlock();
}

int CO_LOCK_OD([[maybe_unused]] CO_CANmodule_t* CANmodule)
{
    FRASY_PROFILE_FUNCTION();
    CO_OD_mutex.lock();
    return 0;    // Always assume success.
}

void CO_UNLOCK_OD([[maybe_unused]] CO_CANmodule_t* CANmodule)
{
    FRASY_PROFILE_FUNCTION();
    CO_OD_mutex.unlock();
}
#endif


/** Disable socketCAN rx ******************************************************/
static CO_ReturnError_t disableRx(CO_CANmodule_t* CANmodule)
{
    /* insert a filter that doesn't match any messages */
    auto* interfaces = static_cast<Frasy::CanOpen::CanOpen::Interfaces_t*>(CANmodule->interface);
    for (auto&& [port, interface] : *interfaces) {
        interface.mute();
    }

    return CO_ERROR_NO;
}


/** Set up or update socketCAN rx filters *************************************/
static CO_ReturnError_t setRxFilters(CO_CANmodule_t* CANmodule)
{
    // Effectively allow everything, since we do not have filtering capabilities.
    auto* interfaces = static_cast<Frasy::CanOpen::CanOpen::Interfaces_t*>(CANmodule->interface);
    for (auto&& [port, interface] : *interfaces) {
        interface.unmute();
    }

    return CO_ERROR_NO;
}


/******************************************************************************/
void CO_CANsetConfigurationMode([[maybe_unused]] void* CANptr)
{
    /* Can't do anything because no reference to CANmodule_t is provided */
}


/******************************************************************************/
void CO_CANsetNormalMode(CO_CANmodule_t* CANmodule)
{
    CO_ReturnError_t ret;

    if (CANmodule != nullptr) {
        CANmodule->CANnormal = false;
        ret                  = setRxFilters(CANmodule);
        if (ret == CO_ERROR_NO) {
            /* Put CAN module in normal mode */
            CANmodule->CANnormal = true;
        }
    }
}


/******************************************************************************/
CO_ReturnError_t CO_CANmodule_init(CO_CANmodule_t*           CANmodule,
                                   void*                     CANptr,
                                   CO_CANrx_t                rxArray[],
                                   uint16_t                  rxSize,
                                   CO_CANtx_t                txArray[],
                                   uint16_t                  txSize,
                                   [[maybe_unused]] uint16_t CANbitRate)
{
    /* verify arguments */
    if (CANmodule == nullptr || CANptr == nullptr || rxArray == nullptr || txArray == nullptr) {
        return CO_ERROR_ILLEGAL_ARGUMENT;
    }

    CANmodule->interface = CANptr;

    /* Configure object variables */
    CANmodule->rxArray        = rxArray;
    CANmodule->rxSize         = rxSize;
    CANmodule->txArray        = txArray;
    CANmodule->txSize         = txSize;
    CANmodule->CANerrorStatus = 0;
    CANmodule->CANnormal      = false;
    CANmodule->CANtxCount     = 0;

    for (uint16_t i = 0U; i < rxSize; i++) {
        rxArray[i].ident          = 0U;
        rxArray[i].mask           = 0xFFFFFFFFU;
        rxArray[i].object         = nullptr;
        rxArray[i].CANrx_callback = nullptr;
    }

    auto* interfaces = static_cast<Frasy::CanOpen::CanOpen::Interfaces_t*>(CANmodule->interface);
    for (auto&& [port, interface] : *interfaces) {
        interface.open();
    }

    return CO_ERROR_NO;
}

/******************************************************************************/
void CO_CANmodule_disable(CO_CANmodule_t* CANmodule)
{
    if (CANmodule == nullptr || CANmodule->interface == nullptr) { return; }

    CANmodule->CANnormal = false;
    auto* interfaces     = static_cast<Frasy::CanOpen::CanOpen::Interfaces_t*>(CANmodule->interface);
    for (auto&& [port, interface] : *interfaces) {
        interface.close();
    }
}


/******************************************************************************/
CO_ReturnError_t CO_CANrxBufferInit(CO_CANmodule_t* CANmodule,
                                    uint16_t        index,
                                    uint16_t        ident,
                                    uint16_t        mask,
                                    bool_t          rtr,
                                    void*           object,
                                    void            (*CANrx_callback)(void* object, void* message))
{
    CO_ReturnError_t ret = CO_ERROR_NO;

    if ((CANmodule != nullptr) && (index < CANmodule->rxSize)) {

        /* buffer, which will be configured */
        CO_CANrx_t* buffer = &CANmodule->rxArray[index];

        /* Configure object variables */
        buffer->object         = object;
        buffer->CANrx_callback = CANrx_callback;

        /* CAN identifier and CAN mask, bit aligned with CAN module */
        buffer->ident = ident & CANID_MASK;
        if (rtr) { buffer->ident |= FLAG_RTR; }
        buffer->mask = (mask & CANID_MASK) | FLAG_RTR;

        /* Set CAN hardware module filter and mask. */
        if (CANmodule->CANnormal) { ret = setRxFilters(CANmodule); }
    }
    else {
        log_printf(LOG_DEBUG, DBG_CAN_RX_PARAM_FAILED, "illegal argument");
        ret = CO_ERROR_ILLEGAL_ARGUMENT;
    }

    return ret;
}

/******************************************************************************/
CO_CANtx_t* CO_CANtxBufferInit(
  CO_CANmodule_t* CANmodule, uint16_t index, uint16_t ident, bool_t rtr, uint8_t noOfBytes, bool_t syncFlag)
{
    CO_CANtx_t* buffer = nullptr;

    if ((CANmodule != nullptr) && (index < CANmodule->txSize)) {
        /* get specific buffer */
        buffer = &CANmodule->txArray[index];

        /* CAN identifier and rtr */
        buffer->ident = ident & CANID_MASK;
        if (rtr) { buffer->ident |= FLAG_RTR; }
        buffer->DLC        = noOfBytes;
        buffer->bufferFull = false;
        buffer->syncFlag   = syncFlag;
    }

    return buffer;
}

/* Change handling of tx buffer full in CO_CANsend(). Use CO_CANtx_t->bufferFull
 * flag. Re-transmit undelivered message inside CO_CANmodule_process(). */
CO_ReturnError_t CO_CANsend(CO_CANmodule_t* CANmodule, CO_CANtx_t* buffer)
{
    FRASY_PROFILE_FUNCTION();
    CO_ReturnError_t err = CO_ERROR_NO;

    if (CANmodule == nullptr || buffer == nullptr) { return CO_ERROR_ILLEGAL_ARGUMENT; }

    auto* interfaces = static_cast<Frasy::CanOpen::CanOpen::Interfaces_t*>(CANmodule->interface);
    if (interfaces == nullptr) { return CO_ERROR_ILLEGAL_ARGUMENT; }


    /* Verify overflow */
    if (buffer->bufferFull) {
        log_printf(LOG_ERR, DBG_CAN_TX_FAILED, buffer->ident, "Buffer full");
        err = CO_ERROR_TX_OVERFLOW;
    }

    auto packet = Frasy::SlCan::Packet {*buffer};
    {
        CO_LOCK_CAN_SEND(CANmodule);
        bool allSuccess = true;
        for (auto&& [port, interface] : *interfaces) {
            size_t written = interface.transmit(packet);
            if (written != packet.sizeOfSerialPacket()) { allSuccess = false; }
        }
        if (allSuccess) {
            if (buffer->bufferFull) {
                buffer->bufferFull = false;
                CANmodule->CANtxCount--;
            }
        }
        else {
            // Send failed, message will be re-sent by CO_CANmodule_process().
            if (!buffer->bufferFull) {
                buffer->bufferFull = true;
                CANmodule->CANtxCount++;
            }
            err = CO_ERROR_TX_BUSY;
        }
        CO_UNLOCK_CAN_SEND(CANmodule);
    }

    return err;
}


/******************************************************************************/
void CO_CANclearPendingSyncPDOs([[maybe_unused]]  CO_CANmodule_t* CANmodule)
{
    /* Messages are either written to the socket queue or dropped */
}


/******************************************************************************/
void CO_CANmodule_process(CO_CANmodule_t* CANmodule)
{
    FRASY_PROFILE_FUNCTION();
    if (CANmodule == nullptr || CANmodule->interface == nullptr) { return; }

    /* recall CO_CANsend(), if message was unsent before */
    if (CANmodule->CANtxCount > 0) {
        bool_t found = false;

        for (uint16_t i = 0; i < CANmodule->txSize; i++) {
            CO_CANtx_t* buffer = &CANmodule->txArray[i];

            if (buffer->bufferFull) {
                buffer->bufferFull = false;
                CANmodule->CANtxCount--;
                CO_CANsend(CANmodule, buffer);
                found = true;
                break;
            }
        }

        if (!found) { CANmodule->CANtxCount = 0; }
    }
}

void CO_CANpollReceive(CO_CANmodule_t* canModule)
{
    FRASY_PROFILE_FUNCTION();
    if (canModule == nullptr || canModule->interface == nullptr) { return; }

    auto* interfaces = static_cast<Frasy::CanOpen::CanOpen::Interfaces_t*>(canModule->interface);

    for (auto&& [port, interface] : *interfaces) {
        while (interface.available() != 0) {
            auto packetOpt = interface.receive().toCOCanRxMsg();
            if (!packetOpt.has_value()) {
                // TODO maybe handle it somehow
                continue;
            }
            auto& packet = packetOpt.value();

            // Message has been received. Search rxArray from CANmodule for the same CAN-ID.
            auto* rcvMsgObj = &canModule->rxArray[0];
            bool  matched   = false;
            for (size_t index = 0; index < canModule->rxSize; index++) {
                if (((packet.ident ^ rcvMsgObj->ident) & rcvMsgObj->mask) == 0) {
                    // Received message matches this "filter".
                    matched = true;
                    break;
                }
                rcvMsgObj++;
            }

            if (matched && (rcvMsgObj != nullptr && rcvMsgObj->CANrx_callback != nullptr)) {
                // Call specific function for that "filter", it will do the processing.
                rcvMsgObj->CANrx_callback(rcvMsgObj->object, &packet);
            }
        }
    }
}
}
// extern "C"
