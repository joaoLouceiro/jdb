#ifndef JDB_TYPES_HPP
#define JDB_TYPES_HPP

#include <array>
#include <cstddef>

namespace jdb {
using byte64 = std::array<std::byte, 8>;
using byte128 = std::array<std::byte, 16>;
} // namespace jdb

#endif // !JDB_TYPES_HPP
