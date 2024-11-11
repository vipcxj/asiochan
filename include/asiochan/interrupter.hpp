#pragma once

#include <mutex>
#include <condition_variable>

namespace asiochan
{
    struct interrupter_t
    {
        std::mutex mux;
        std::condition_variable cv;
        bool interrupted = false;
        bool available = true;

        bool interrupt()
        {
            std::lock_guard g(mux);
            if (available)
            {
                interrupted = true;
                cv.notify_all();
                return true;
            }
            else
            {
                return false;
            }
        }

    };
} // namespace asiochan
