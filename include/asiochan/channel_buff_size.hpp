#pragma once

#include <cstddef>
#include <limits>
#include <type_traits>

namespace asiochan
{
    using channel_buff_size = std::size_t;

    inline constexpr auto unbounded_channel_buff = std::numeric_limits<channel_buff_size>::max();

    constexpr bool is_unbounded(channel_buff_size buffer_size)
    {
        return buffer_size == unbounded_channel_buff;
    }

    template<channel_buff_size buff_size>
    struct is_not_zero : std::true_type {};

    template<>
    struct is_not_zero<0> : std::false_type {};

}  // namespace asiochan
