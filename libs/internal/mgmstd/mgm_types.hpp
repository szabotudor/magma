#pragma once
#include <cstdint>
#include <functional>


namespace mgm {
    class MId {
      public:
#if INTPTR_MAX == INT64_MAX
        using _uint = uint64_t;
        using _int = int64_t;
#elif INTPTR_MAX == INT32_MAX
        using _uint = uint32_t;
        using _int = int32_t;
#else
#error "Unsupported platform"
#endif
        _uint id = 0;

        constexpr explicit MId(_uint value = 0)
            : id(value) {}
        constexpr operator _uint() const { return id; }
        constexpr operator _int() const { return (_int)id; }
        constexpr operator bool() const { return id != 0; }

        constexpr _uint& operator*() { return id; }
        constexpr const _uint& operator*() const { return id; }
        constexpr _uint* operator->() { return &id; }
        constexpr const _uint* operator->() const { return &id; }

        constexpr bool operator==(const MId& other) const { return id == other.id; }
        constexpr bool operator!=(const MId& other) const { return id != other.id; }
        constexpr bool operator<(const MId& other) const { return id < other.id; }
        constexpr bool operator>(const MId& other) const { return id > other.id; }
        constexpr bool operator<=(const MId& other) const { return id <= other.id; }
        constexpr bool operator>=(const MId& other) const { return id >= other.id; }
        constexpr MId& operator++() {
            ++id;
            return *this;
        }
        constexpr MId operator++(int) {
            MId tmp(*this);
            operator++();
            return tmp;
        }
        constexpr MId& operator--() {
            --id;
            return *this;
        }
        constexpr MId operator--(int) {
            MId tmp(*this);
            operator--();
            return tmp;
        }
        constexpr MId& operator+=(const MId& other) {
            id += other.id;
            return *this;
        }
        constexpr MId& operator-=(const MId& other) {
            id -= other.id;
            return *this;
        }
        constexpr MId& operator*=(const MId& other) {
            id *= other.id;
            return *this;
        }
        constexpr MId& operator/=(const MId& other) {
            id /= other.id;
            return *this;
        }
        constexpr MId& operator%=(const MId& other) {
            id %= other.id;
            return *this;
        }

        constexpr MId operator+(const MId& other) const { return MId(id + other.id); }
        constexpr MId operator-(const MId& other) const { return MId(id - other.id); }
        constexpr MId operator*(const MId& other) const { return MId(id * other.id); }
        constexpr MId operator/(const MId& other) const { return MId(id / other.id); }
        constexpr MId operator%(const MId& other) const { return MId(id % other.id); }
        constexpr MId operator&(const MId& other) const { return MId(id & other.id); }
        constexpr MId operator|(const MId& other) const { return MId(id | other.id); }
        constexpr MId operator^(const MId& other) const { return MId(id ^ other.id); }
        constexpr MId operator<<(const MId& other) const { return MId(id << other.id); }
        constexpr MId operator>>(const MId& other) const { return MId(id >> other.id); }
        constexpr MId operator&&(const MId& other) const { return MId(id && other.id); }
        constexpr MId operator||(const MId& other) const { return MId(id || other.id); }
    };
} // namespace mgm

namespace std {
    template<>
    struct hash<mgm::MId> {
        std::size_t operator()(const mgm::MId& id) const noexcept {
            return std::hash<mgm::MId::_uint>{}(id.id);
        }
    };
} // namespace std