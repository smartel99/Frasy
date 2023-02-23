/**
 * @file    response.cpp
 * @author  Paul Thomas
 * @date    2023-02-21
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

#include "response.h"

#include <utility>

namespace Frasy::Communication
{

ResponsePromise::~ResponsePromise()
{
    if (Thread.joinable()) { Thread.join(); }
}

ResponsePromise& ResponsePromise::OnComplete(const on_complete_cb_t& func)
{
    if (func) { m_onCompleteCb = func; }
    else { throw std::bad_function_call(); }
    return *this;
}

ResponsePromise& ResponsePromise::OnTimeout(const on_timeout_cb_t& func)
{
    if (func) { m_onTimeoutCb = func; }
    else { throw std::bad_function_call(); }
    return *this;
}

ResponsePromise& ResponsePromise::OnError(const on_error_cb_t& func)
{
    if (func) { m_onErrorCb = func; }
    else { throw std::bad_function_call(); }
    return *this;
}

void ResponsePromise::Async()
{
    run();
}

void ResponsePromise::Await()
{
    std::atomic_flag completed;
    m_localOnCompleteCb = [&](const Packet& pkt)
    {
        m_onCompleteCb(pkt);
        completed.test_and_set();
        completed.notify_all();
    };
    run();
    completed.wait(false);
}

void ResponsePromise::run()
{
    using namespace std::chrono_literals;
    auto s_future = Promise.get_future();
    Thread        = std::thread(
      [this](std::future<Packet> future)
      {
          using namespace std::chrono_literals;
          constexpr auto s_timeout = 1000ms;
          try
          {
              std::future_status status = future.wait_for(s_timeout);
              switch (status)
              {
                  case std::future_status::ready:
                  case std::future_status::deferred:
                      // Result only available when explicitly requested.
                      m_localOnCompleteCb(future.get());
                      break;
                  case std::future_status::timeout: m_localOnTimeoutCb(); break;
              }
          }
          catch (const std::exception& e)
          {
              m_localOnErrorCb(e);
          }
      },
      std::move(s_future));
}

}    // namespace Frasy::Communication