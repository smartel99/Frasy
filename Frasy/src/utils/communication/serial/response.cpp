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

namespace Frasy::Serial {

ResponsePromise::~ResponsePromise()
{
    if (m_thread.joinable())
    {
        BR_LOG_DEBUG(s_tag, "Joining thread...");
        m_thread.join();
        BR_LOG_DEBUG(s_tag, "Thread joined!");
    }
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
    std::atomic<bool> completed = false;
    m_localOnCompleteCb         = [&](const Packet& pkt)
    {
        if (m_onCompleteCb) m_onCompleteCb(pkt);
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
}

void ResponsePromise::run()
{
    using namespace std::chrono_literals;
    auto s_future = Promise.get_future();
    m_thread      = std::thread(
      [this](std::future<Packet> future)
      {
          try
          {
              std::future_status status = future.wait_for(s_timeout);
              switch (status)
              {
                  case std::future_status::ready:
                  case std::future_status::deferred:
                  {
                      // Result only available when explicitly requested.
                      Packet packet = future.get();
                      m_localOnCompleteCb(packet);
                  }
                  break;
                  case std::future_status::timeout:
                  {
                      BR_LOG_WARN(s_tag, "Promise timed out!");
                      OnTimeoutCb();
                  }
                  break;
              }
          }
          catch (...)
          {
              m_localOnErrorCb(std::current_exception());
          }
          m_consumed = true;
      },
      std::move(s_future));
}

}    // namespace Frasy::Communication
