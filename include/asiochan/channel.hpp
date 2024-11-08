#pragma once

#include <concepts>
#include <memory>
#if defined(ASIOCHAN_CH_ALLOCATE_TRACER) && defined(ASIOCHAN_CH_ALLOCATE_TRACER_FULL)
#include <source_location>
#endif

#include "asiochan/asio.hpp"
#include "asiochan/channel_buff_size.hpp"
#include "asiochan/channel_concepts.hpp"
#include "asiochan/detail/allocate_tracer.hpp"
#include "asiochan/detail/channel_method_ops.hpp"
#include "asiochan/detail/channel_shared_state.hpp"
#include "asiochan/sendable.hpp"

namespace asiochan
{
    enum class channel_stream_mode : unsigned
    {
        block_until_available = 0,
        forget_oldest = 1
    };

    constexpr channel_flags make_channel_flags(channel_flags flags, channel_stream_mode stream_mode)
    {
        if (static_cast<bool>(stream_mode))
        {
            return static_cast<channel_flags>(flags | channel_flags::forget_oldest);
        }
        else
        {
            return flags;
        }
    }

    template <sendable T,
              channel_buff_size buff_size_,
              channel_flags flags_,
              asio::execution::executor Executor>
    class channel_base
    {
      public:
        using executor_type = Executor;
        using shared_state_type = detail::channel_shared_state<T, Executor, buff_size_, flags_is_forget_oldest(flags_)>;
        using shared_state_ptr_type = std::shared_ptr<shared_state_type>;
        using send_type = T;

        static constexpr auto flags = flags_;
        static constexpr auto buff_size = buff_size_;

        static_assert(!flags_is_forget_oldest(flags) or buff_size > 0, "The buff_size must greater than zero when stream_mode is forget_oldest");

        [[nodiscard]] channel_base(
#if defined(ASIOCHAN_CH_ALLOCATE_TRACER) && defined(ASIOCHAN_CH_ALLOCATE_TRACER_FULL)
            const std::source_location& src_loc
#endif
            )
          : shared_state_{std::make_shared<shared_state_type>(
#if defined(ASIOCHAN_CH_ALLOCATE_TRACER) && defined(ASIOCHAN_CH_ALLOCATE_TRACER_FULL)
                src_loc
#endif
                )}
        {
        }

        // clang-format off
        template <channel_flags other_flags>
        requires (flags_convertable_to(other_flags, flags))
        [[nodiscard]] channel_base(
            channel_base<T, buff_size_, other_flags, Executor> const& other) noexcept
          : shared_state_{other.shared_state_}
        // clang-format on
        {
        }

        // clang-format off
        template <channel_flags other_flags>
        requires (flags_convertable_to(other_flags, flags))
        [[nodiscard]] channel_base(
            channel_base<T, buff_size_, other_flags, Executor>&& other) noexcept
          : shared_state_{std::move(other.shared_state_)}
        // clang-format on
        {
        }

        template <channel_flags other_flags>
        requires (flags_convertable_to(other_flags, flags))
        channel_base<T, buff_size_, flags_, Executor>& operator=(channel_base<T, buff_size_, other_flags, Executor> const& other) noexcept
        {
            shared_state_ = other.shared_state_;
            return *this;
        }

        template <channel_flags other_flags>
        requires (flags_convertable_to(other_flags, flags))
        channel_base<T, buff_size_, flags_, Executor>& operator=(channel_base<T, buff_size_, other_flags, Executor>&& other) noexcept
        {
            shared_state_ = std::move(other.shared_state_);
            return *this;
        }

        [[nodiscard]] auto shared_state() noexcept -> shared_state_type&
        {
            return *shared_state_;
        }

        [[nodiscard]] auto shared_state() const noexcept -> const shared_state_type&
        {
            return *shared_state_;
        }

        [[nodiscard]] auto shared_state_ptr() const noexcept -> const shared_state_ptr_type&
        {
            return shared_state_;
        }

        [[nodiscard]] friend auto operator==(
            channel_base const& lhs,
            channel_base const& rhs) noexcept -> bool
                                                 = default;

      protected:
        ~channel_base() noexcept = default;

      private:
        template <sendable, channel_buff_size, channel_flags, asio::execution::executor>
        friend class channel_base;

        shared_state_ptr_type shared_state_;
    };

    template <sendable T, channel_buff_size buff_size, channel_stream_mode stream_mode, asio::execution::executor Executor>
    class basic_channel
      : public channel_base<T, buff_size, make_channel_flags(bidirectional, stream_mode), Executor>,
        public detail::channel_method_ops<T, Executor, buff_size, make_channel_flags(bidirectional, stream_mode), basic_channel<T, buff_size, stream_mode, Executor>>
    {
      private:
        using base = channel_base<T, buff_size, make_channel_flags(bidirectional, stream_mode), Executor>;
        using ops = detail::channel_method_ops<T, Executor, buff_size, make_channel_flags(bidirectional, stream_mode), basic_channel<T, buff_size, stream_mode, Executor>>;

      public:
        using base::base;
        using base::operator=;

        using ops::try_read;

        using ops::read;

        using ops::try_write;

        using ops::write;

#if defined(ASIOCHAN_CH_ALLOCATE_TRACER) && defined(ASIOCHAN_CH_ALLOCATE_TRACER_FULL)
        basic_channel(const std::source_location& src_loc = std::source_location::current())
          : base(src_loc) { }
#endif
    };

    template <sendable T, channel_buff_size buff_size, channel_stream_mode stream_mode, asio::execution::executor Executor>
    class basic_read_channel
      : public channel_base<T, buff_size, make_channel_flags(readable, stream_mode), Executor>,
        public detail::channel_method_ops<T, Executor, buff_size, make_channel_flags(readable, stream_mode), basic_read_channel<T, buff_size, stream_mode, Executor>>
    {
      private:
        using base = channel_base<T, buff_size, make_channel_flags(readable, stream_mode), Executor>;
        using ops = detail::channel_method_ops<T, Executor, buff_size, make_channel_flags(readable, stream_mode), basic_read_channel<T, buff_size, stream_mode, Executor>>;

      public:
        using base::base;

        using ops::try_read;

        using ops::read;

#if defined(ASIOCHAN_CH_ALLOCATE_TRACER) && defined(ASIOCHAN_CH_ALLOCATE_TRACER_FULL)
        basic_read_channel(const std::source_location& src_loc = std::source_location::current())
          : base(src_loc) { }
#endif
    };

    template <sendable T, channel_buff_size buff_size, channel_stream_mode stream_mode, asio::execution::executor Executor>
    class basic_write_channel
      : public channel_base<T, buff_size, make_channel_flags(writable, stream_mode), Executor>,
        public detail::channel_method_ops<T, Executor, buff_size, make_channel_flags(writable, stream_mode), basic_write_channel<T, buff_size, stream_mode, Executor>>
    {
      private:
        using base = channel_base<T, buff_size, make_channel_flags(writable, stream_mode), Executor>;
        using ops = detail::channel_method_ops<T, Executor, buff_size, make_channel_flags(writable, stream_mode), basic_write_channel<T, buff_size, stream_mode, Executor>>;

      public:
        using base::base;

        using ops::try_write;

        using ops::write;

#if defined(ASIOCHAN_CH_ALLOCATE_TRACER) && defined(ASIOCHAN_CH_ALLOCATE_TRACER_FULL)
        basic_write_channel(const std::source_location& src_loc = std::source_location::current())
          : base(src_loc) { }
#endif
    };

    template <sendable T, channel_buff_size buff_size = 0, channel_stream_mode stream_mode = channel_stream_mode::block_until_available>
    using channel = basic_channel<T, buff_size, stream_mode, asio::any_io_executor>;

    template <sendable T, channel_buff_size buff_size = 0, channel_stream_mode stream_mode = channel_stream_mode::block_until_available>
    using read_channel = basic_read_channel<T, buff_size, stream_mode, asio::any_io_executor>;

    template <sendable T, channel_buff_size buff_size = 0, channel_stream_mode stream_mode = channel_stream_mode::block_until_available>
    using write_channel = basic_write_channel<T, buff_size, stream_mode, asio::any_io_executor>;

    template <sendable T, channel_buff_size buff_size = 1>
    requires is_not_zero<buff_size>::value
    using unblocked_channel = channel<T, buff_size, channel_stream_mode::forget_oldest>;

    template <sendable T, channel_buff_size buff_size = 1>
    requires is_not_zero<buff_size>::value
    using unblocked_read_channel = read_channel<T, buff_size, channel_stream_mode::forget_oldest>;

    template <sendable T, channel_buff_size buff_size = 1>
    requires is_not_zero<buff_size>::value
    using unblocked_write_channel = write_channel<T, buff_size, channel_stream_mode::forget_oldest>;

    template <sendable T>
    using unbounded_channel = channel<T, unbounded_channel_buff>;

    template <sendable T>
    using unbounded_read_channel = read_channel<T, unbounded_channel_buff>;

    template <sendable T>
    using unbounded_write_channel = write_channel<T, unbounded_channel_buff>;
}  // namespace asiochan
