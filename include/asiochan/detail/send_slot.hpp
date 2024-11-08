#pragma once

#include <cassert>
#include <optional>
#include <utility>

#include "asiochan/sendable.hpp"

namespace asiochan::detail
{
    template <sendable T>
    class send_slot
    {
      public:
        auto read() noexcept -> T
        {
            assert(value_.has_value());
            return *std::exchange(value_, {});
        }

        void write(T&& value) noexcept
        {
            assert(not value_.has_value());
            value_.emplace(std::move(value));
        }

        auto value() noexcept -> std::optional<T>&
        {
            return value_;
        }

        auto value() const noexcept -> std::optional<T> const&
        {
            return value_;
        }

        template<bool override = false>
        friend void transfer(send_slot& from, send_slot& to) noexcept
        {
            assert(from.value_.has_value());
            if constexpr(!override)
            {
                assert(not to.value_.has_value());
            }
            to.value_.swap(from.value_);
            if constexpr(!override)
            {
                assert(not from.value_.has_value());
            }
            else
            {
                from.value_.reset();
            }
        }

      private:
        std::optional<T> value_ = std::nullopt;
    };

    template <>
    class send_slot<void>
    {
      public:
        static void read() noexcept { }

        static void write() noexcept { }

        template<bool override = false>
        friend void transfer(send_slot&, send_slot&) noexcept
        {
        }
    };
}  // namespace asiochan::detail
