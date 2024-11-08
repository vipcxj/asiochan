#pragma once

#include <concepts>

#include "asiochan/detail/channel_shared_state.hpp"

namespace asiochan
{
    enum channel_flags : unsigned
    {
        readable = 1u << 0u,
        writable = 1u << 1u,
        forget_oldest = 1u << 2u,
        bidirectional = readable | writable,
    };

    constexpr bool flags_is_readable(channel_flags flags)
    {
        return static_cast<bool>(flags & readable);
    }

    constexpr bool flags_is_writable(channel_flags flags)
    {
        return static_cast<bool>(flags & writable);
    }

    constexpr bool flags_is_forget_oldest(channel_flags flags)
    {
        return static_cast<bool>(flags & forget_oldest);
    }

    constexpr bool flags_convertable_to(channel_flags from, channel_flags to)
    {
        if (flags_is_readable(to) && !flags_is_readable(from))
        {
            return false;
        }
        if (flags_is_writable(to) && !flags_is_writable(from))
        {
            return false;
        }
        return flags_is_forget_oldest(from) == flags_is_forget_oldest(to);
    }

    // clang-format off
    template <typename T>
    concept any_channel_type = requires (T& channel, T const& const_channel)
    {
        typename T::executor_type;
        requires asio::execution::executor<typename T::executor_type>;

        typename T::shared_state_type;
        typename T::send_type;

        requires std::is_same_v<std::decay_t<decltype(T::flags)>, channel_flags>;
        requires std::is_integral_v<decltype(T::buff_size)>;

        requires detail::channel_shared_state_type<
                     typename T::shared_state_type,
                     typename T::send_type,
                     typename T::executor_type>;

        { channel.shared_state_ptr() } noexcept -> std::same_as<const typename T::shared_state_ptr_type&>;
    };

    template <typename T>
    concept any_readable_channel_type
        = any_channel_type<T> and flags_is_readable(T::flags);

    template <typename T>
    concept any_writable_channel_type
        = any_channel_type<T> and flags_is_writable(T::flags);

    template <typename T>
    concept any_bidirectional_channel_type
        = any_readable_channel_type<T> and any_writable_channel_type<T>;

    template <typename T>
    concept any_unblocked_channel_type = any_channel_type<T> and flags_is_forget_oldest(T::flags);

    template <typename T>
    concept any_unblocked_writable_channel_type = any_writable_channel_type<T> and flags_is_forget_oldest(T::flags);

    template <typename T>
    concept any_unblocked_readable_channel_type = any_readable_channel_type<T> and flags_is_forget_oldest(T::flags);

    template <typename T>
    concept any_unblocked_bidirectional_channel_type = any_bidirectional_channel_type<T> and flags_is_forget_oldest(T::flags);

    template <typename T>
    concept any_unbounded_channel_type = any_channel_type<T> and is_unbounded(T::buff_size);

    template <typename T>
    concept any_unbounded_writable_channel_type = any_writable_channel_type<T> and is_unbounded(T::buff_size);

    template <typename T>
    concept any_unbounded_readable_channel_type = any_readable_channel_type<T> and is_unbounded(T::buff_size);

    template <typename T>
    concept any_unbounded_bidirectional_channel_type = any_bidirectional_channel_type<T> and is_unbounded(T::buff_size);

    template <typename T, typename SendType>
    concept channel_type
        = any_channel_type<T> and std::same_as<SendType, typename T::send_type>;

    template <typename T, typename SendType>
    concept readable_channel_type
        = channel_type<T, SendType> and any_readable_channel_type<T>;

    template <typename T, typename SendType>
    concept writable_channel_type
        = channel_type<T, SendType> and any_writable_channel_type<T>;

    template <typename T, typename SendType>
    concept bidirectional_channel_type
        = channel_type<T, SendType> and any_bidirectional_channel_type<T>;

    template <typename T, typename SendType>
    concept unblocked_channel_type
        = any_unblocked_channel_type<T> and std::same_as<SendType, typename T::send_type>;

    template <typename T, typename SendType>
    concept unblocked_readable_channel_type
        = channel_type<T, SendType> and any_unblocked_readable_channel_type<T>;

    template <typename T, typename SendType>
    concept unblocked_writable_channel_type
        = channel_type<T, SendType> and any_unblocked_writable_channel_type<T>;

    template <typename T, typename SendType>
    concept unblocked_bidirectional_channel_type
        = channel_type<T, SendType> and any_unblocked_bidirectional_channel_type<T>;

    template <typename T, typename SendType>
    concept unbounded_channel_type
        = any_unbounded_channel_type<T> and std::same_as<SendType, typename T::send_type>;

    template <typename T, typename SendType>
    concept unbounded_readable_channel_type
        = channel_type<T, SendType> and any_unbounded_readable_channel_type<T>;

    template <typename T, typename SendType>
    concept unbounded_writable_channel_type
        = channel_type<T, SendType> and any_unbounded_writable_channel_type<T>;

    template <typename T, typename SendType>
    concept unbounded_bidirectional_channel_type
        = channel_type<T, SendType> and any_unbounded_bidirectional_channel_type<T>;
    // clang-format on
}  // namespace asiochan
