#pragma once

#include <concepts>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace loadicator {

class LoadicatorError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

template <std::invocable Action>
decltype(auto) withContext(std::string_view context, Action&& action) {
    try {
        return std::forward<Action>(action)();
    } catch (const LoadicatorError& error) {
        throw LoadicatorError(std::string(context) + ": " + error.what());
    }
}

}
