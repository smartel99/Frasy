/**
 * @file    response.h
 * @author  Samuel Martel
 * @date    2022-12-13
 * @brief
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 * You should have received a copy of the GNU General Public License along with this program. If
 * not, see <a href=https://www.gnu.org/licenses/>https://www.gnu.org/licenses/</a>.
 */

#ifndef FRASY_UTILS_COMMUNICATION_RESPONSE_H
#define FRASY_UTILS_COMMUNICATION_RESPONSE_H

#include "packet.h"

#include <atomic>
#include <Brigerad/Core/Core.h>
#include <Brigerad/Core/Log.h>
#include <chrono>
#include <functional>
#include <future>
#include <thread>
#include <utility>


namespace Frasy::Communication
{
struct ResponsePromise
{
    std::promise<Packet> Promise;
    std::thread          Thread;

    explicit ResponsePromise()                    = default;
    ResponsePromise(ResponsePromise&&)            = default;
    ResponsePromise& operator=(ResponsePromise&&) = default;
    ~ResponsePromise();

    using on_complete_cb_t = std::function<void(const Packet&)>;
    using on_timeout_cb_t  = std::function<void()>;
    using on_error_cb_t    = std::function<void(const std::exception&)>;

    ResponsePromise& OnComplete(const on_complete_cb_t& func);
    ResponsePromise& OnTimeout(const on_timeout_cb_t& func);
    ResponsePromise& OnError(const on_error_cb_t& func);

    /// Run the promise in asynchronous mode
    void Async();

    /// Run the promise in synchronous mode
    void Await();

    /// Run the promise in synchronous mode
    /// Immediately collect and convert the response
    template<typename T>
    T Collect()
    {
        Packet           packet;
        std::atomic_flag completed;
        m_localOnCompleteCb = [&](Packet pkt)
        {
            packet = std::move(pkt);
            completed.test_and_set();
            completed.notify_all();
        };
        run();
        completed.wait(false);
        return packet.FromPayload<T>();
    }

    /// Run promise in synchronous mode
    /// Immediately collect the packet
    template<>
    Packet Collect()
    {
        Packet           packet;
        std::atomic_flag completed;
        m_localOnCompleteCb = [&](Packet pkt)
        {
            packet = std::move(pkt);
            completed.test_and_set();
            completed.notify_all();
        };
        run();
        completed.wait(false);
        return packet;
    }


private:
    static constexpr const char* s_tag = "Promise";

    on_complete_cb_t m_localOnCompleteCb = [&](const Packet& packet)
    {
        if (m_onCompleteCb) { m_onCompleteCb(packet); }
    };
    on_timeout_cb_t m_localOnTimeoutCb = [&]()
    {
        if (m_onTimeoutCb) { m_onTimeoutCb(); }
        else { throw std::exception("Timeout"); }
    };
    on_error_cb_t m_localOnErrorCb = [&](const std::exception& e)
    {
        if (m_onErrorCb) { m_onErrorCb(e); }
        else { throw e; }
    };

    on_complete_cb_t m_onCompleteCb;
    on_timeout_cb_t  m_onTimeoutCb;
    on_error_cb_t    m_onErrorCb;

    void run();
};
}    // namespace Frasy::Communication

#endif    // FRASY_UTILS_COMMUNICATION_RESPONSE_H
