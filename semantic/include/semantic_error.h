#pragma once

#include <stdexcept>
#include <string>

namespace olang {

class SemanticError : public std::runtime_error {
    std::string message_;

public:
    explicit SemanticError(const std::string& message)
        : std::runtime_error(message), message_(message) {}

    const std::string& message() const { return message_; }
};

}
