#pragma once
#include <cstdint>


class ID_t {
public:
#if INTPTR_MAX == INT64_MAX
    using _uint = uint64_t;
#elif INTPTR_MAX == INT32_MAX
    using _uint_t = uint32_t;
#else
    #error "Unsupported platform"
#endif
    _uint id = 0;

    constexpr explicit ID_t(_uint id = 0) : id(id) {}
    constexpr operator _uint() const { return id; }
    constexpr operator bool() const { return id != 0; }

    constexpr _uint& operator*() { return id; }
    constexpr const _uint& operator*() const { return id; }
    constexpr _uint* operator->() { return &id; }
    constexpr const _uint* operator->() const { return &id; }

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
    constexpr ID_t operator&&(const ID_t& other) const { return ID_t(id && other.id); }
    constexpr ID_t operator||(const ID_t& other) const { return ID_t(id || other.id); }
};
