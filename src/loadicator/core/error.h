#pragma once

#include <stdexcept>
#include <string>

namespace loadicator {

/// The single exception type thrown by the library. Messages carry their context
/// ("file:line: what went wrong"); callers add context rather than replacing the message.
class LoadicatorError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

} // namespace loadicator
