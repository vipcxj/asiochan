#pragma once

#include <concepts>
#include <cstddef>
#include <mutex>
#include <condition_variable>
#include <variant>

#include <iostream>

#include "asiochan/async_promise.hpp"
#include "asiochan/sync_promise.hpp"
#include "asiochan/detail/overloaded.hpp"
#include "asiochan/detail/send_slot.hpp"
#include "asiochan/sendable.hpp"

namespace asiochan::detail
{
    using select_waiter_token = std::size_t;

    struct select_sync_t {};
    inline constexpr select_sync_t select_sync_tag {};

    struct select_async_t {};
    inline constexpr select_async_t select_async_tag {};

    template <asio::execution::executor Executor>
    struct select_wait_context
    {
        using promise_t = std::variant<async_promise<select_waiter_token, Executor>, sync_promise<select_waiter_token>>;
        using async_promise_t = async_promise<select_waiter_token, Executor>;
        using sync_promise_t = sync_promise<select_waiter_token>;
        promise_t promise;

        std::mutex mutex;
        bool avail_flag = true;

        select_wait_context(const select_sync_t &, interrupter_t & interrupter): promise(std::in_place_type<sync_promise_t>, interrupter) {}

        select_wait_context(const select_async_t &): promise(std::in_place_type<async_promise_t>) {}

        void set_token(select_waiter_token token)
        {
            std::visit(detail::overloaded{
                [&token](async_promise<select_waiter_token, Executor> & promise)
                {
                    promise.set_value(token);
                },
                [&token](sync_promise<select_waiter_token> & promise)
                {
                    promise.set_value(token);
                },
            }, promise);
        }

        async_promise_t & get_async_promise()
        {
            return std::get<async_promise_t>(promise);
        }

        const async_promise_t & get_async_promise() const
        {
            return std::get<async_promise_t>(promise);
        }

        sync_promise_t & get_sync_promise()
        {
            return std::get<sync_promise_t>(promise);
        }

        const sync_promise_t & get_sync_promise() const
        {
            return std::get<sync_promise_t>(promise);
        }
    };

    template <asio::execution::executor Executor>
    auto claim(select_wait_context<Executor>& ctx) -> bool
    {
        auto const lock = std::scoped_lock{ctx.mutex};
        return std::exchange(ctx.avail_flag, false);
    }

    template <sendable T, asio::execution::executor Executor>
    struct channel_waiter_list_node
    {
        select_wait_context<Executor>* ctx = nullptr;
        send_slot<T>* slot = nullptr;
        select_waiter_token token = 0;
        channel_waiter_list_node* prev = nullptr;
        channel_waiter_list_node* next = nullptr;
    };

    template <sendable T, asio::execution::executor Executor>
    void notify_waiter(channel_waiter_list_node<T, Executor>& waiter)
    {
        waiter.ctx->set_token(waiter.token);
    }

    template <sendable T, asio::execution::executor Executor>
    class channel_waiter_list
    {
      public:
        using node_type = channel_waiter_list_node<T, Executor>;

        void enqueue(node_type& node) noexcept
        {
            node.prev = last_;
            node.next = nullptr;

            if (not first_)
            {
                first_ = &node;
            }
            else
            {
                last_->next = &node;
            }

            last_ = &node;
        }

        void dequeue(node_type& node) noexcept
        {
            assert(node.prev || &node == first_ || (!node.prev && !node.next)); // node may have been removed
            assert(node.next || &node == last_ || (!node.prev && !node.next)); // node may have been removed
            if (&node == first_)
            {
                first_ = node.next;
            }
            if (&node == last_)
            {
                last_ = node.prev;
            }
            if (node.prev)
            {
                node.prev->next = node.next;
            }
            if (node.next)
            {
                node.next->prev = node.prev;
            }
            if (node.prev)
            {
                node.prev = nullptr;
            }
            if (node.next)
            {
                node.next = nullptr;
            }
        }

        auto dequeue_first_available(
            std::same_as<select_wait_context<Executor>> auto&... contexts) noexcept
            -> node_type*
        {
            while (first_)
            {
                auto const node = first_;

                auto const pop = [&]()
                {
                    first_ = node->next;
                    if (not first_)
                    {
                        last_ = nullptr;
                    }
                    else
                    {
                        first_->prev = nullptr;
                        node->next = nullptr;
                    }
                };

                auto const lock = std::scoped_lock{node->ctx->mutex, contexts.mutex...};
                if (node->ctx->avail_flag)
                {
                    if (not(contexts.avail_flag and ...))
                    {
                        return nullptr;
                    }

                    node->ctx->avail_flag = false;
                    ((contexts.avail_flag = false), ...);

                    pop();

                    return node;
                }

                pop();
            }

            return nullptr;
        }

        void print() const {
            std::cout << "waiter list " << this << ":";
            auto node = first_;
            while (node)
            {
                std::cout << " " << node;
                node = node->next;
            }
            std::cout << std::endl;
        }

      private:
        node_type* first_ = nullptr;
        node_type* last_ = nullptr;
    };
}  // namespace asiochan::detail
