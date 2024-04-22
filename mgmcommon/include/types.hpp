#pragma once
#include <cstdint>


class ID_t {
#if INTPTR_MAX == INT64_MAX
    using uint = uint64_t;
#elif INTPTR_MAX == INT32_MAX
    using uint_t = uint32_t;
#else
    #error "Unsupported platform"
#endif
    uint id = 0;

public:
    constexpr explicit ID_t(uint id = 0) : id(id) {}
    constexpr operator uint() const { return static_cast<uint>(id); }

    constexpr uint& operator*() { return id; }
    constexpr const uint& operator*() const { return id; }
    constexpr uint* operator->() { return &id; }
    constexpr const uint* operator->() const { return &id; }

    constexpr bool operator==(const ID_t& other) const { return id == other.id; }
    constexpr bool operator!=(const ID_t& other) const { return id != other.id; }
    constexpr bool operator<(const ID_t& other) const { return id < other.id; }
    constexpr bool operator>(const ID_t& other) const { return id > other.id; }
    constexpr bool operator<=(const ID_t& other) const { return id <= other.id; }
    constexpr bool operator>=(const ID_t& other) const { return id >= other.id; }
    constexpr ID_t& operator++() { ++id; return *this; }
    constexpr ID_t operator++(int) { ID_t tmp(*this); operator++(); return tmp; }
    constexpr ID_t& operator--() { --id; return *this; }
    constexpr ID_t operator--(int) { ID_t tmp(*this); operator--(); return tmp; }
    constexpr ID_t& operator+=(const ID_t& other) { id += other.id; return *this; }
    constexpr ID_t& operator-=(const ID_t& other) { id -= other.id; return *this; }
    constexpr ID_t& operator*=(const ID_t& other) { id *= other.id; return *this; }
    constexpr ID_t& operator/=(const ID_t& other) { id /= other.id; return *this; }
    constexpr ID_t& operator%=(const ID_t& other) { id %= other.id; return *this; }

    constexpr ID_t operator+(const ID_t& other) const { return ID_t(id + other.id); }
    constexpr ID_t operator-(const ID_t& other) const { return ID_t(id - other.id); }
    constexpr ID_t operator*(const ID_t& other) const { return ID_t(id * other.id); }
    constexpr ID_t operator/(const ID_t& other) const { return ID_t(id / other.id); }
    constexpr ID_t operator%(const ID_t& other) const { return ID_t(id % other.id); }
    constexpr ID_t operator&(const ID_t& other) const { return ID_t(id & other.id); }
    constexpr ID_t operator|(const ID_t& other) const { return ID_t(id | other.id); }
    constexpr ID_t operator^(const ID_t& other) const { return ID_t(id ^ other.id); }
    constexpr ID_t operator<<(const ID_t& other) const { return ID_t(id << other.id); }
    constexpr ID_t operator>>(const ID_t& other) const { return ID_t(id >> other.id); }
    constexpr ID_t operator~() const { return ID_t(~id); }
    constexpr ID_t operator-() const { return ID_t(-id); }
    constexpr ID_t operator+() const { return ID_t(+id); }
    constexpr ID_t operator!() const { return ID_t(!id); }
    constexpr ID_t operator&&(const ID_t& other) const { return ID_t(id && other.id); }
    constexpr ID_t operator||(const ID_t& other) const { return ID_t(id || other.id); }
};
