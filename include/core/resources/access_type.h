#pragma once

enum struct Access: unsigned int {
    Read = 1,
    Write = 2,
    ReadWrite = 3
};

constexpr inline Access operator |(Access l, Access r) {
    return static_cast<Access>(static_cast<unsigned int>(l) | static_cast<unsigned int>(r));
}

constexpr inline Access operator &(Access l, Access r) {
    return static_cast<Access>(static_cast<unsigned int>(l) & static_cast<unsigned int>(r));
}