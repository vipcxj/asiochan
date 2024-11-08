#pragma once

#include <mutex>
#include <condition_variable>

namespace asiochan
{
    struct interrupter_t
    {
        std::condition_variable cv;
        bool interrupted = false;

        void interrupt()
        {
            interrupted = true;
            cv.notify_all();
        }
    };
} // namespace asiochan
