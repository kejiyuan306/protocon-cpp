#pragma once

#include <cstdint>
#include <functional>

namespace Protocon {

struct ClientToken {
    uint64_t value;

    bool operator==(const ClientToken& other) const { return value == other.value; }
};

}  // namespace Protocon

namespace std {

template <>
struct hash<Protocon::ClientToken> {
    std::size_t operator()(const Protocon::ClientToken& tk) const {
        return std::hash<uint64_t>()(tk.value);
    }
};

}  // namespace std
