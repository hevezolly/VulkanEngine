#pragma once

#include <common.h>
#include <functional>

template<typename F, typename... CallArgs>
void invoke_and_check(F&& f, CallArgs&&... args) {
    if constexpr (std::is_void<std::invoke_result_t<F, CallArgs...>>::value) {
        std::invoke(std::forward<F>(f), std::forward<CallArgs>(args)...);
    } else {
        auto result = std::invoke(std::forward<F>(f), std::forward<CallArgs>(args)...);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("vk collect failed");
        }
    }
}

template<typename T, typename F, typename... Args>
API std::vector<T> vkCollect(F&& f, Args&&... args) {
    uint32_t count;

    invoke_and_check(std::forward<F>(f), std::forward<Args>(args)..., &count, static_cast<T*>(nullptr));

    std::vector<T> result(count);

    if (count == 0) {
        return result;
    }
    
    invoke_and_check(std::forward<F>(f), std::forward<Args>(args)..., &count, result.data());

    return result;
}