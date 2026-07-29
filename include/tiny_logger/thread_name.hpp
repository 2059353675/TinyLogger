#pragma once

#include <cstdint>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>

namespace tiny_logger {

namespace detail {
    inline std::unordered_map<uint64_t, std::string>& thread_name_map() {
        static std::unordered_map<uint64_t, std::string> map;
        return map;
    }

    inline std::shared_mutex& thread_name_mutex() {
        static std::shared_mutex mtx;
        return mtx;
    }

    inline const char*& thread_name_cache() {
        thread_local const char* name = nullptr;
        return name;
    }
} // namespace detail

inline void set_thread_name(const char* name) {
    detail::thread_name_cache() = (name && name[0] != '\0') ? name : nullptr;

    auto tid = std::hash<std::thread::id>{}(std::this_thread::get_id());
    auto& map = detail::thread_name_map();
    std::lock_guard<std::shared_mutex> lock(detail::thread_name_mutex());

    if (name && name[0] != '\0') {
        map[tid] = name;
    } else {
        map.erase(tid);
    }
}

inline const char* get_thread_name() {
    return detail::thread_name_cache();
}

inline const char* resolve_thread_name(uint64_t tid) {
    auto& map = detail::thread_name_map();
    std::shared_lock<std::shared_mutex> lock(detail::thread_name_mutex());

    auto it = map.find(tid);
    return (it != map.end()) ? it->second.c_str() : nullptr;
}

} // namespace tiny_logger
