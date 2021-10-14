#pragma once

#include <cstdint>
#include <utility>

namespace Protocon {

struct ClientToken {
    uint64_t value;

    bool operator==(const ClientToken& other) const { return value == other.value; }
};

}  // namespace Protocon

template <>
struct std::hash<Protocon::ClientToken> {
    std::size_t operator()(const Protocon::ClientToken& tk) const {
        return std::hash<uint64_t>()(tk.value);
    }
};
