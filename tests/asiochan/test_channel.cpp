#include <numeric>
#include <ranges>
#include <string>
#include <atomic>

#include <iostream>

#include <asiochan/channel.hpp>
#include "catch2/catch_all.hpp"

#ifdef ASIOCHAN_USE_STANDALONE_ASIO

#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/io_context.hpp>
#include <asio/thread_pool.hpp>
#include <asio/use_future.hpp>
#include <asio/steady_timer.hpp>

#else

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/asio/steady_timer.hpp>

#endif

namespace asio = asiochan::asio;

auto asleep(std::chrono::nanoseconds dur) -> asio::awaitable<void>
{
    auto executor = co_await asio::this_coro::executor;
    auto timer = asio::steady_timer{executor};
    timer.expires_after(dur);
    co_await timer.async_wait(asio::use_awaitable);
    co_return;
}

auto avoid() -> asio::awaitable<void>
{
    co_return;
}

template<asio::execution::executor Executor>
auto make_timeout(std::chrono::nanoseconds dur, Executor executor) -> asiochan::channel<void, 1>
{
    asiochan::channel<void, 1> ch {};
    asio::co_spawn(std::move(executor), [dur, ch]() -> asio::awaitable<void> {
        co_await asleep(dur);
        co_await ch.write();
    }, asio::detached);
    return ch;
}

TEST_CASE("Channels")
{
    auto const num_threads = GENERATE(range(10u, 20u));
    auto thread_pool = asio::thread_pool{num_threads};

    SECTION("Ping-pong")
    {
        auto channel = asiochan::channel<std::string>{};

        auto ping_task = asio::co_spawn(
            thread_pool,
            [channel]() -> asio::awaitable<void>
            {
                co_await channel.write("ping");
                auto const recv = co_await channel.read();
                CHECK(recv == "pong");
            },
            asio::use_future);

        auto pong_task = asio::co_spawn(
            thread_pool,
            [channel]() -> asio::awaitable<void>
            {
                auto const recv = co_await channel.read();
                CHECK(recv == "ping");
                co_await channel.write("pong");
            },
            asio::use_future);

        pong_task.get();
        ping_task.get();
    }

    SECTION("Buffered channel")
    {
        static constexpr auto buffer_size = 3;

        auto channel = asiochan::channel<int, buffer_size>{};
        auto read_channel = asiochan::read_channel<int, buffer_size>{channel};
        auto write_channel = asiochan::write_channel<int, buffer_size>{channel};

        for (auto const i : std::views::iota(0, buffer_size))
        {
            auto const was_sent = write_channel.try_write(i);
            CHECK(was_sent);
        }
        auto const last_was_sent = write_channel.try_write(0);
        CHECK(not last_was_sent);

        for (auto const i : std::views::iota(0, buffer_size))
        {
            auto const recv = read_channel.try_read();
            REQUIRE(recv.has_value());
            CHECK(*recv == i);
        }
        auto const last_recv = read_channel.try_read();
        CHECK(not last_recv.has_value());
    }

    SECTION("Concurrence buffered channel")
    {
        static constexpr auto buffer_size = 1;

        auto channel = asiochan::channel<int, buffer_size>{};
        std::atomic_int sum {0};
        for (size_t i = 0; i < 100; i++)
        {
            asio::co_spawn(
                thread_pool,
                [channel, i]() -> asio::awaitable<void>
                {
                    co_await channel.write(i);
                },
                asio::use_future);
            asio::co_spawn(
                thread_pool,
                [channel, &sum]() -> asio::awaitable<void>
                {
                    auto i = co_await channel.read();
                    ++sum;
                },
                asio::use_future);
        }
        thread_pool.join();
        CHECK(sum.load() == 100);
    }

    SECTION("Buffered channel of void")
    {
        static constexpr auto buffer_size = 3;

        auto channel = asiochan::channel<void, buffer_size>{};
        auto read_channel = asiochan::read_channel<void, buffer_size>{channel};
        auto write_channel = asiochan::write_channel<void, buffer_size>{channel};

        for (auto const i : std::views::iota(0, buffer_size))
        {
            auto const was_sent = write_channel.try_write();
            CHECK(was_sent);
        }
        auto const last_was_sent = write_channel.try_write();
        CHECK(not last_was_sent);

        for (auto const i : std::views::iota(0, buffer_size))
        {
            auto const recv = read_channel.try_read();
            CHECK(recv);
        }
        auto const last_recv = read_channel.try_read();
        CHECK(not last_recv);
    }

    SECTION("Unbounded buffered channel")
    {
        static constexpr auto num_tokens = 10;

        auto channel = asiochan::unbounded_channel<int>{};
        auto read_channel = asiochan::unbounded_read_channel<int>{channel};
        auto write_channel = asiochan::unbounded_write_channel<int>{channel};

        for (auto const i : std::views::iota(0, num_tokens))
        {
            write_channel.write(i);
        }

        for (auto const i : std::views::iota(0, num_tokens))
        {
            auto const recv = read_channel.try_read();
            CHECK(recv == i);
        }
        auto const last_recv = read_channel.try_read();
        CHECK(not last_recv.has_value());
    }

    SECTION("Channel of channel")
    {
        using CH0 = asiochan::channel<int>;
        asiochan::channel<CH0> ch_of_ch0;
    }

    SECTION("Multiple writers and receivers")
    {
        static constexpr auto num_tokens_per_task = 5;
        static constexpr auto num_tasks = 3;

        auto channel = asiochan::channel<int>{};
        auto read_channel = asiochan::read_channel<int>{channel};
        auto write_channel = asiochan::write_channel<int>{channel};

        auto source_values = std::vector<int>(num_tasks * num_tokens_per_task);
        std::iota(source_values.begin(), source_values.end(), 0);

        auto source_tasks = std::vector<std::future<void>>{};
        for (auto const task_id : std::views::iota(0, num_tasks))
        {
            source_tasks.push_back(
                asio::co_spawn(
                    thread_pool,
                    [write_channel, task_id, &source_values]() -> asio::awaitable<void>
                    {
                        auto const start = task_id * num_tokens_per_task;
                        for (auto const i : std::views::iota(start, start + num_tokens_per_task))
                        {
                            co_await write_channel.write(source_values[i]);
                        }
                    },
                    asio::use_future));
        }

        auto sink_values = std::vector<int>(num_tasks * num_tokens_per_task);
        auto sink_tasks = std::vector<std::future<void>>{};
        for (auto const task_id : std::views::iota(0, num_tasks))
        {
            sink_tasks.push_back(
                asio::co_spawn(
                    thread_pool,
                    [read_channel, task_id, &sink_values]() -> asio::awaitable<void>
                    {
                        auto const start = task_id * num_tokens_per_task;
                        for (auto const i : std::views::iota(start, start + num_tokens_per_task))
                        {
                            sink_values[i] = co_await read_channel.read();
                        }
                    },
                    asio::use_future));
        }

        for (auto& sink_task : sink_tasks)
        {
            sink_task.get();
        }

        std::ranges::sort(sink_values);
        CHECK(source_values == sink_values);

        for (auto& source_task : source_tasks)
        {
            source_task.get();
        }
    }

    SECTION("Make sure crash when channel_waiter_list.dequeue not happen again")
    {
        using namespace asiochan;
        auto task = asio::co_spawn(
            thread_pool,
            []() -> asio::awaitable<void>
            {
                auto executor = co_await asio::this_coro::executor;
                channel<int, 1> ch {};
                for (size_t i = 0; i < 2; i++)
                {
                    unbounded_channel<void> res {};
                    for (size_t i = 0; i < 3; i++)
                    {
                        asio::co_spawn(executor, [ch, res]() -> asio::awaitable<void>{
                            auto executor = co_await asio::this_coro::executor;
                            auto timeouter = make_timeout(std::chrono::milliseconds(30), executor);
                            co_await asiochan::select(
                                asiochan::ops::read(timeouter),
                                asiochan::ops::read(ch)
                            );
                            res.write();
                        }, asio::detached);
                    }
                    for (size_t i = 0; i < 3; i++)
                    {
                        co_await res.read();
                    }
                }
            },
            asio::use_future
        );
        task.get();
    }
}
