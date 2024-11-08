#pragma once

#include <optional>
#include <utility>

#include "asiochan/asio.hpp"
#include "asiochan/channel_buff_size.hpp"
#include "asiochan/channel_concepts.hpp"
#include "asiochan/nothing_op.hpp"
#include "asiochan/read_op.hpp"
#include "asiochan/select.hpp"
#include "asiochan/sendable.hpp"
#include "asiochan/write_op.hpp"

namespace asiochan::detail
{
    template <sendable T,
              asio::execution::executor Executor,
              channel_buff_size buff_size,
              channel_flags flags,
              typename Derived>
    class channel_method_ops
    {
      public:
        // clang-format off
        [[nodiscard]] auto try_read() const -> std::optional<T>
        requires (flags_is_readable(flags))
        // clang-format on
        {
            auto result = select_ready(
                ops::read(derived()),
                ops::nothing);

            if (auto const ptr = result.template get_if_received<T>())
            {
                return std::move(*ptr);
            }

            return std::nullopt;
        }

        // clang-format off
        [[nodiscard]] auto try_write(T value) const -> bool
        requires (flags_is_writable(flags) and !flags_is_forget_oldest(flags) and !is_unbounded(buff_size))
        // clang-format on
        {
            auto const result = select_ready(
                ops::write(std::move(value), derived()),
                ops::nothing);

            return result.has_value();
        }

        // clang-format off
        [[nodiscard]] auto read() const -> asio::awaitable<T, Executor>
        requires (flags_is_readable(flags))
        // clang-format on
        {
            auto result = co_await select(ops::read(derived()));

            co_return std::move(result).template get_received<T>();
        }

        auto read_sync() const -> T
        requires (flags_is_readable(flags))
        {
            auto result = select_sync(ops::read(derived()));
            return std::move(result).template get_received<T>();
        }

        // clang-format off
        [[nodiscard]] auto write(T value) const -> asio::awaitable<void, Executor>
        requires (flags_is_writable(flags) and !flags_is_forget_oldest(flags) and !is_unbounded(buff_size))
        // clang-format on
        {
            co_await select(ops::write(std::move(value), derived()));
        }

        void write_sync(T value) const
        requires (flags_is_writable(flags) and !flags_is_forget_oldest(flags) and !is_unbounded(buff_size))
        {
            select_sync(ops::write(std::move(value), derived()));
        }

        // clang-format off
        void write(T value) const
        requires (flags_is_writable(flags) and (flags_is_forget_oldest(flags) or is_unbounded(buff_size)))
        // clang-format on
        {
            select_ready(ops::write(std::move(value), derived()));
        }

      private:
        [[nodiscard]] auto derived() noexcept -> Derived&
        {
            return static_cast<Derived&>(*this);
        }

        [[nodiscard]] auto derived() const noexcept -> const Derived&
        {
            return static_cast<const Derived&>(*this);
        }
    };

    template <channel_buff_size buff_size,
              asio::execution::executor Executor,
              channel_flags flags,
              typename Derived>
    class channel_method_ops<void, Executor, buff_size, flags, Derived>
    {
      public:
        // clang-format off
        [[nodiscard]] auto try_read() const -> bool
        requires (flags_is_readable(flags))
        // clang-format on
        {
            auto const result = select_ready(
                ops::read(derived()),
                ops::nothing);

            return result.has_value();
        }

        // clang-format off
        [[nodiscard]] auto try_write() const -> bool
        requires (flags_is_writable(flags) and !flags_is_forget_oldest(flags) and !is_unbounded(buff_size))
        // clang-format on
        {
            auto const result = select_ready(
                ops::write(derived()),
                ops::nothing);

            return result.has_value();
        }

        // clang-format off
        [[nodiscard]] auto read() const -> asio::awaitable<void>
        requires (flags_is_readable(flags))
        // clang-format on
        {
            co_await select(ops::read(derived()));
        }

        void read_sync() const
        requires (flags_is_readable(flags))
        {
            select_sync(ops::read(derived()));
        }

        // clang-format off
        [[nodiscard]] auto write() const -> asio::awaitable<void>
        requires (flags_is_writable(flags) and !flags_is_forget_oldest(flags) and !is_unbounded(buff_size))
        // clang-format on
        {
            co_await select(ops::write(derived()));
        }

        void write_sync() const
        requires (flags_is_writable(flags) and !flags_is_forget_oldest(flags) and !is_unbounded(buff_size))
        {
            select_sync(ops::write(derived()));
        }

        // clang-format off
        void write() const
        requires (flags_is_writable(flags) and (flags_is_forget_oldest(flags) or is_unbounded(buff_size)))
        // clang-format on
        {
            select_ready(ops::write(derived()));
        }

      private:
        [[nodiscard]] auto derived() noexcept -> Derived&
        {
            return static_cast<Derived&>(*this);
        }

        [[nodiscard]] auto derived() const noexcept -> const Derived&
        {
            return static_cast<const Derived&>(*this);
        }
    };
}  // namespace asiochan::detail
