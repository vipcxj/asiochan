#pragma once

#include "asiochan/interrupter.hpp"
#include "asiochan/sendable.hpp"

#include <optional>

namespace asiochan
{
    template <sendable T>
    struct sync_promise
    {
        std::optional<T> value = std::nullopt;
        interrupter_t & interrupter;

        sync_promise(interrupter_t & interrupter_): interrupter(interrupter_) {}

        template <std::convertible_to<T> U>
        void set_value(U&& value)
        {
            this->value = T{std::forward<U>(value)};
            interrupter.cv.notify_one();
        }

        /**
         * @return true if value is set, or interrupted
         */
        bool wait(std::unique_lock<std::mutex> & lk)
        {
            interrupter.cv.wait(lk, [this]() {
                return value.has_value() || interrupter.interrupted;
            });
            return !interrupter.interrupted;
        }
    };

    template<>
    struct sync_promise<void>
    {
        bool done = false;
        interrupter_t & interrupter;

        sync_promise(interrupter_t & interrupter_): interrupter(interrupter_) {}

        void set_value()
        {
            done = true;
            interrupter.cv.notify_one();
        }

        /**
         * @return true if value is set, or interrupted
         */
        bool wait(std::unique_lock<std::mutex> & lk)
        {
            interrupter.cv.wait(lk, [this]() {
                return done || interrupter.interrupted;
            });
            return !interrupter.interrupted;
        }
    };
} // namespace asiochan
