#pragma once

#include <atomic>
#if defined(ASIOCHAN_CH_ALLOCATE_TRACER) && defined(ASIOCHAN_CH_ALLOCATE_TRACER_FULL)
#include <mutex>
#include <source_location>
#include <unordered_map>
#include <vector>
#include <functional>
#include <algorithm>
#include <ranges>
#endif

namespace asiochan
{

struct source_location_pool
{
    struct KeyHasher
    {
        std::size_t operator()(const std::source_location & loc) const
        {
            using std::size_t;
            using std::hash;
            size_t res = 17;
            res = res * 31 + hash<std::uintptr_t>()(reinterpret_cast<std::uintptr_t>(loc.file_name()));
            res = res * 31 + hash<std::uintptr_t>()(reinterpret_cast<std::uintptr_t>(loc.function_name()));
            res = res * 31 + hash<decltype(loc.line())>()(loc.line());
            res = res * 31 + hash<decltype(loc.column())>()(loc.column());
            return res;
        }
    };

    struct KeyEqualer
    {
        bool operator()(const std::source_location & loc1, const std::source_location & loc2) const
        {
            return loc1.file_name() == loc2.file_name()
                && loc1.function_name() == loc2.function_name()
                && loc1.line() == loc2.line()
                && loc1.column() == loc2.column();
        }
    };

    using id_type = std::size_t;
    using pool_type = std::unordered_map<std::source_location, id_type, KeyHasher, KeyEqualer>;
    using locs_type = std::vector<std::source_location>;

    struct entries
    {
        std::mutex &mux;
        pool_type &pool;
        locs_type &locs;

        id_type get_id(const std::source_location & loc)
        {
            std::lock_guard lk(mux);
            id_type id;
            auto iter = pool.find(loc);
            if (iter == pool.end())
            {
                id = locs.size();
                locs.push_back(loc);
                pool[loc] = id;
            }
            else
            {
                id = iter->second;
            }
            return id;
        }

        const std::source_location & get_loc(id_type id) const
        {
            std::lock_guard lk(mux);
            return locs[id];
        }
    };

    static entries get() noexcept
    {
        static std::mutex g_mux {};
        static pool_type g_pool {};
        static locs_type g_locs {};
        return { g_mux, g_pool, g_locs };
    } 
};

using loc_id_type = source_location_pool::id_type;

#if defined(ASIOCHAN_CH_ALLOCATE_TRACER) && defined(ASIOCHAN_CH_ALLOCATE_TRACER_FULL)
template <std::ranges::forward_range R, typename Cmp = std::ranges::greater>
constexpr std::vector<std::ranges::borrowed_iterator_t<R>>
max_n_elements(R &&range, size_t n, Cmp compare) {
    // The iterator type of the input range
    using iter_t = std::ranges::borrowed_iterator_t<R>;
    // The range of iterators over the input range
    auto iterators = std::views::iota(std::ranges::begin(range), 
                                    std::ranges::end(range));
    // Vector of iterators to the largest n elements
    std::vector<iter_t> result(n);
    // Sort the largest n elements of the input range, and store iterators to 
    // these elements to the result vector
    std::ranges::partial_sort_copy(iterators, result, compare);
    return result;
}
#endif

class allocate_tracer
{
private:

    struct tracer_entry
    {
        loc_id_type ctr_loc_id;
    };

public:
    using tracer_entries_type = std::unordered_map<std::uintptr_t, tracer_entry>;
    using tracer_locs_type = std::unordered_map<loc_id_type, std::size_t>;
    using tracer_mutex_type = std::mutex;

private:
    struct tracer_ref
    {
        std::atomic_int64_t& ch_ref_count;
#ifdef ASIOCHAN_CH_ALLOCATE_TRACER_FULL
        tracer_mutex_type& mux;
        tracer_entries_type& entries;
        tracer_locs_type& locs;
#endif
    };

    static tracer_ref global_tracer() noexcept
    {
        static std::atomic_int64_t g_ch_ref_count;
#ifdef ASIOCHAN_CH_ALLOCATE_TRACER_FULL
        static tracer_mutex_type g_mutex;
        static tracer_entries_type g_entries;
        static tracer_locs_type g_locs;
#endif
        return {
            g_ch_ref_count
#ifdef ASIOCHAN_CH_ALLOCATE_TRACER_FULL
            , g_mutex
            , g_entries
            , g_locs
#endif
        };
    }
public:
    static void ctor(
#if defined(ASIOCHAN_CH_ALLOCATE_TRACER) && defined(ASIOCHAN_CH_ALLOCATE_TRACER_FULL)
        std::uintptr_t key,
        const std::source_location & src_loc
#endif
    ) noexcept
    {
#ifdef ASIOCHAN_CH_ALLOCATE_TRACER
        auto tracer = global_tracer();
        tracer.ch_ref_count ++;
#ifdef ASIOCHAN_CH_ALLOCATE_TRACER_FULL
        std::lock_guard lk(tracer.mux);
        auto ctr_loc_id = source_location_pool::get().get_id(src_loc);
        tracer.entries[key] = { ctr_loc_id };
        auto iter = tracer.locs.find(ctr_loc_id);
        if (iter == tracer.locs.end())
        {
            tracer.locs[ctr_loc_id] = 1;
        }
        else
        {
            ++ iter->second;
        }
#endif
#endif
    }

    static void dtor(
#if defined(ASIOCHAN_CH_ALLOCATE_TRACER) && defined(ASIOCHAN_CH_ALLOCATE_TRACER_FULL)
        std::uintptr_t key
#endif
    ) noexcept
    {
#ifdef ASIOCHAN_CH_ALLOCATE_TRACER
        auto tracer = global_tracer();
        tracer.ch_ref_count --;

#ifdef ASIOCHAN_CH_ALLOCATE_TRACER_FULL
        std::lock_guard lk(tracer.mux);
        auto iter = tracer.entries.find(key);
        if (iter != tracer.entries.end())
        {
            auto ctr_id = iter->second.ctr_loc_id;
            tracer.entries.erase(iter);
            auto & ctr_ref_count = tracer.locs[ctr_id];
            -- ctr_ref_count;
            if (ctr_ref_count == 0)
            {
                tracer.locs.erase(ctr_id);
            }
        }
#endif
#endif
    }

#ifdef ASIOCHAN_CH_ALLOCATE_TRACER
    static std::int64_t ref_count() noexcept
    {
        return global_tracer().ch_ref_count.load();
    }

#ifdef ASIOCHAN_CH_ALLOCATE_TRACER_FULL
    static std::optional<std::source_location> get_ctr_loc(std::uintptr_t key)
    {
        auto tracer = global_tracer();
        std::lock_guard lk(tracer.mux);
        auto iter = tracer.entries.find(key);
        if (iter != tracer.entries.end())
        {
            return source_location_pool::get().get_loc(iter->second.ctr_loc_id);
        }
        else
        {
            return std::nullopt;
        }
    }

    static void collect_ctr_src_locs_with_max_n_ref_count(std::size_t n, std::vector<std::pair<loc_id_type, std::size_t>> & locs_ref_result)
    {
        auto tracer = global_tracer();
        std::lock_guard lk(tracer.mux);
        using iter_t = std::ranges::borrowed_iterator_t<decltype(tracer.locs)>;
        locs_ref_result.clear();
        for (auto & iter : max_n_elements(tracer.locs, n, [](iter_t iter1, iter_t iter2) -> bool {
            return iter1->second > iter2->second;
        }))
        {
            locs_ref_result.push_back(std::make_pair(iter->first, iter->second));
        }
    }
#endif

#endif
};

}