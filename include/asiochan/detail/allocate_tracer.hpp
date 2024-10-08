#pragma once

#include <atomic>

namespace asiochan
{

class allocate_tracer
{
private:

    struct tracer_ref
    {
        std::atomic_int64_t& ch_ref_count;
    };

    static tracer_ref global_tracer() noexcept
    {
        static std::atomic_int64_t global_ch_ref_count;
        return { global_ch_ref_count };
    }
public:
    static void ctor() noexcept
    {
        global_tracer().ch_ref_count ++;
    }

    static void dtor() noexcept
    {
        global_tracer().ch_ref_count --;
    }

#ifdef ASIOCHAN_CH_ALLOCATE_TRACER
    static std::int64_t ref_count() noexcept
    {
        return global_tracer().ch_ref_count.load();
    }
#endif
};

}