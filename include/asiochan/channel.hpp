#pragma once

#include <concepts>
#include <memory>

#include "asiochan/asio.hpp"
#include "asiochan/channel_buff_size.hpp"
#include "asiochan/channel_concepts.hpp"
#include "asiochan/detail/channel_method_ops.hpp"
#include "asiochan/detail/channel_shared_state.hpp"
#include "asiochan/sendable.hpp"

namespace asiochan
{
    template <sendable T,
              channel_buff_size buff_size_,
              channel_flags flags_,
              asio::execution::executor Executor>
    class channel_base
    {
      public:
        using executor_type = Executor;
        using shared_state_type = detail::channel_shared_state<T, Executor, buff_size_>;
        using shared_state_ptr_type = std::shared_ptr<shared_state_type>;
        using send_type = T;

        static constexpr auto flags = flags_;
        static constexpr auto buff_size = buff_size_;

        [[nodiscard]] channel_base()
          : shared_state_{std::make_shared<shared_state_type>()}
        {
        }

        [[nodiscard]] channel_base(
            channel_base<T, buff_size_, flags_, Executor> const& other) noexcept
          : shared_state_{other.shared_state_}
        {
        }

        // clang-format off
        template <channel_flags other_flags>
        requires (other_flags != flags && (other_flags & flags) == flags)
        [[nodiscard]] channel_base(
            channel_base<T, buff_size_, other_flags, Executor> const& other) noexcept
          : shared_state_{other.shared_state_}
        // clang-format on
        {
        }

        [[nodiscard]] channel_base(
            channel_base<T, buff_size_, flags_, Executor>&& other) noexcept
          : shared_state_{std::move(other.shared_state_)}
        {
        }

        // clang-format off
        template <channel_flags other_flags>
        requires (other_flags != flags && (other_flags & flags) == flags)
        [[nodiscard]] channel_base(
            channel_base<T, buff_size_, other_flags, Executor>&& other) noexcept
          : shared_state_{std::move(other.shared_state_)}
        // clang-format on
        {
        }

        channel_base<T, buff_size_, flags_, Executor>& operator = (channel_base<T, buff_size_, flags_, Executor> const& other) noexcept = default;

        template <channel_flags other_flags>
        requires (other_flags != flags && (other_flags & flags) == flags)
        channel_base<T, buff_size_, flags_, Executor>& operator = (channel_base<T, buff_size_, other_flags, Executor> const& other) noexcept
        {
            shared_state_ = other.shared_state_;
            return *this;
        }

        channel_base<T, buff_size_, flags_, Executor>& operator = (channel_base<T, buff_size_, flags_, Executor> && other) noexcept = default;

        template <channel_flags other_flags>
        requires (other_flags != flags && (other_flags & flags) == flags)
        channel_base<T, buff_size_, flags_, Executor>& operator = (channel_base<T, buff_size_, other_flags, Executor> && other) noexcept
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

    template <sendable T, channel_buff_size buff_size, asio::execution::executor Executor>
    class basic_channel
      : public channel_base<T, buff_size, bidirectional, Executor>,
        public detail::channel_method_ops<T, Executor, buff_size, bidirectional, basic_channel<T, buff_size, Executor>>
    {
      private:
        using base = channel_base<T, buff_size, bidirectional, Executor>;
        using ops = detail::channel_method_ops<T, Executor, buff_size, bidirectional, basic_channel<T, buff_size, Executor>>;

      public:
        using base::base;
        using base::operator =;

        using ops::try_read;

        using ops::read;

        using ops::try_write;

        using ops::write;
    };

    template <sendable T, channel_buff_size buff_size, asio::execution::executor Executor>
    class basic_read_channel
      : public channel_base<T, buff_size, readable, Executor>,
        public detail::channel_method_ops<T, Executor, buff_size, readable, basic_read_channel<T, buff_size, Executor>>
    {
      private:
        using base = channel_base<T, buff_size, readable, Executor>;
        using ops = detail::channel_method_ops<T, Executor, buff_size, readable, basic_read_channel<T, buff_size, Executor>>;

      public:
        using base::base;

        using ops::try_read;

        using ops::read;
    };

    template <sendable T, channel_buff_size buff_size, asio::execution::executor Executor>
    class basic_write_channel
      : public channel_base<T, buff_size, writable, Executor>,
        public detail::channel_method_ops<T, Executor, buff_size, writable, basic_write_channel<T, buff_size, Executor>>
    {
      private:
        using base = channel_base<T, buff_size, writable, Executor>;
        using ops = detail::channel_method_ops<T, Executor, buff_size, writable, basic_write_channel<T, buff_size, Executor>>;

      public:
        using base::base;

        using ops::try_write;

        using ops::write;
    };

    template <sendable T, channel_buff_size buff_size = 0>
    using channel = basic_channel<T, buff_size, asio::any_io_executor>;

    template <sendable T, channel_buff_size buff_size = 0>
    using read_channel = basic_read_channel<T, buff_size, asio::any_io_executor>;

    template <sendable T, channel_buff_size buff_size = 0>
    using write_channel = basic_write_channel<T, buff_size, asio::any_io_executor>;

    template <sendable T>
    using unbounded_channel = channel<T, unbounded_channel_buff>;

    template <sendable T>
    using unbounded_read_channel = read_channel<T, unbounded_channel_buff>;

    template <sendable T>
    using unbounded_write_channel = write_channel<T, unbounded_channel_buff>;
}  // namespace asiochan
