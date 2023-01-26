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


namespace Frasy::Communication
{
class TransmissionCallbacks
{
public:
    TransmissionCallbacks() noexcept                                   = default;
    TransmissionCallbacks(TransmissionCallbacks&&) noexcept            = default;
    TransmissionCallbacks& operator=(TransmissionCallbacks&&) noexcept = default;

    using on_complete_cb_t = std::function<void(Packet)>;
    using on_timeout_cb_t  = std::function<void()>;
    using on_error_cb_t    = std::function<void()>;

    TransmissionCallbacks& OnComplete(const on_complete_cb_t& func)
    {
        if (func) { m_onCompleteCb = func; }
        else { throw std::bad_function_call(); }
    }
    TransmissionCallbacks& OnTimeout(const on_timeout_cb_t& func)
    {
        if (func) { m_onTimeoutCb = func; }
        else { throw std::bad_function_call(); }
    }

    TransmissionCallbacks& OnError(const on_error_cb_t& func)
    {
        if (func) { m_onErrorCb = func; }
        else { throw std::bad_function_call(); }
    }

    template<typename T>
    T Await()
    {
        BR_ASSERT(!m_onCompleteCb, "Await cannot be called after OnComplete!");
        std::atomic_flag ready;
        Packet           packet;
        m_onCompleteCb = [&](Packet pkt)
        {
            packet = pkt;
            ready.test_and_set();
            ready.notify_all();
        };
        ready.wait(false);
        return packet.FromPayload<T>();
    }

private:
    on_complete_cb_t m_onCompleteCb = {};
    on_timeout_cb_t  m_onTimeoutCb  = []() { BR_LOG_WARN(s_tag, "Timed out"); };
    on_error_cb_t    m_onErrorCb    = []() { BR_LOG_ERROR(s_tag, "An error occurred."); };


    static constexpr const char* s_tag = "Transmission callbacks";

    friend class ResponsePromise;
};


struct ResponsePromise
{
    TransmissionCallbacks Callbacks;
    std::promise<Packet>  Promise;
    std::thread           Thread;

    explicit ResponsePromise()
    {
        using namespace std::chrono_literals;
        constexpr auto s_timeout = 100ms;
        Thread                   = std::thread(
          [s_timeout, this]()
          {
              try
              {
                  std::future<Packet> future = Promise.get_future();
                  std::future_status  status = future.wait_for(s_timeout);
                  switch (status)
                  {
                      case std::future_status::ready:
                      case std::future_status::deferred:    // Result only available when explicitly requested.
                          Callbacks.m_onCompleteCb(future.get());
                          break;
                      case std::future_status::timeout: Callbacks.m_onTimeoutCb(); break;
                  }
              }
              catch (std::future_error& e)
              {
                  BR_LOG_ERROR("ResponsePromise", "A future error occurred: {}", e.what());
                  Callbacks.m_onErrorCb();
              }
          });
    }
    ResponsePromise(ResponsePromise&&)            = default;
    ResponsePromise& operator=(ResponsePromise&&) = default;

    ~ResponsePromise()
    {
        if (Thread.joinable()) { Thread.join(); }
    }
};
}    // namespace Frasy::Communication

#endif    // FRASY_UTILS_COMMUNICATION_RESPONSE_H
