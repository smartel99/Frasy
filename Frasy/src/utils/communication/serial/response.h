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

    explicit ResponsePromise()                         = default;
    ResponsePromise(ResponsePromise&&)                 = default;
    ResponsePromise(const ResponsePromise&)            = delete;
    ResponsePromise& operator=(ResponsePromise&&)      = default;
    ResponsePromise& operator=(const ResponsePromise&) = delete;
    ~ResponsePromise();

    using on_complete_cb_t = std::function<void(const Packet&)>;
    using on_timeout_cb_t  = std::function<void()>;
    using on_error_cb_t    = std::function<void(const std::exception&)>;

    ResponsePromise& OnComplete(const on_complete_cb_t& func);
    ResponsePromise& OnTimeout(const on_timeout_cb_t& func);
    ResponsePromise& OnError(const on_error_cb_t& func);

    bool IsConsumed() const { return m_consumed; }

    /// Run the promise in asynchronous mode
    void Async();

    /// Run the promise in synchronous mode
    void Await();

    /// Run the promise in synchronous mode
    /// Immediately collect and convert the response
    template<typename T = Packet>
    T Collect()
    {
        Packet            packet;
        std::atomic<bool> completed;
        m_localOnCompleteCb = [&](Packet pkt)
        {
            packet    = std::move(pkt);
            completed = true;
            completed.notify_all();
        };
        m_localOnErrorCb = [&](std::exception_ptr e)
        {
            OnErrorCb(e);
            completed = true;
            completed.notify_all();
        };
        run();
        completed.wait(false);
        if constexpr (std::is_same_v<T, Packet>) { return packet; }
        else { return packet.FromPayload<T>(); }
    }

private:
    static constexpr const char* s_tag     = "Promise";
    static constexpr auto        s_timeout = std::chrono::milliseconds(5000);

    std::thread m_thread;
    bool        m_consumed = false;

    on_complete_cb_t m_localOnCompleteCb = [&](const Packet& packet)
    {
        if (m_onCompleteCb) { m_onCompleteCb(packet); }
    };
    std::function<void(std::exception_ptr)> m_localOnErrorCb = [&](std::exception_ptr e) { OnErrorCb(e); };

    on_complete_cb_t m_onCompleteCb;
    on_timeout_cb_t  m_onTimeoutCb;
    on_error_cb_t    m_onErrorCb;

    void run();

    void OnTimeoutCb()
    {
        if (m_onTimeoutCb) { m_onTimeoutCb(); }
        else { throw std::runtime_error("Timed out"); }
    }
    void OnErrorCb(std::exception_ptr eptr)
    {
        if (m_onErrorCb)
        {
            try
            {
                if (eptr) { std::rethrow_exception(eptr); }
                else { throw std::runtime_error("No information provided"); }
            }
            catch (const std::exception& e)
            {
                m_onErrorCb(e);
            }
        }
        else { Promise.set_exception(eptr); }
    }
};
}    // namespace Frasy::Communication

#endif    // FRASY_UTILS_COMMUNICATION_RESPONSE_H
