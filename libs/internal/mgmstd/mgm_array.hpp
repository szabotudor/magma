#pragma once
#include <cstring>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>

#include "mgm_allocator.hpp"
#include "mgm_assert.hpp"
#include "mgmath.hpp"


namespace mgm {
    template<typename T, typename Allocator = MAlloc<T>>
    class MArray {
        T* _data = nullptr;
        usize _capacity = 0, _size = 0;

      public:
        struct Iterator {
            using value_type = T;
            using difference_type = usize;
            using pointer = T*;
            using reference = T&;

            MArray<T, Allocator>* arr = nullptr;
            usize p = 0;

            Iterator(MArray<T, Allocator>& source_array = nullptr, usize loc = 0)
                : arr(&source_array),
                  p(loc) {}

            Iterator(const Iterator& other) = default;
            Iterator(Iterator&& other)
                : arr(other.arr),
                  p(other.p) {
                other = {};
            }
            Iterator& operator=(const Iterator& other) {
                if (this == &other)
                    return *this;

                arr = other.arr;
                p = other.p;

                return *this;
            }
            Iterator& operator=(Iterator&& other) {
                if (this == &other)
                    return *this;

                arr = other.arr;
                p = other.p;
                other = {};

                return *this;
            }

            Iterator& operator++() {
                p = min(p + 1, arr->size());
                return *this;
            }
            Iterator& operator--() {
                if (p == 0)
                    return *this;

                p = max(p - 1, 0);
            }

            Iterator operator++(int) {
                Iterator cpy{*this};
                ++*this;
                return cpy;
            }
            Iterator operator--(int) {
                Iterator cpy{*this};
                --*this;
                return cpy;
            }

            Iterator operator+(isize i) const {
                return Iterator{arr, clamp(static_cast<isize>(0), arr->_size - 1, i)};
            }
            Iterator operator-(isize i) const {
                return Iterator{arr, clamp(static_cast<isize>(0), arr->_size - 1, i)};
            }

            Iterator& operator+=(isize i) {
                return *this = *this + i;
            }
            Iterator& operator-=(isize i) {
                return *this = *this - i;
            }

            bool operator==(const Iterator& other) const {
                return arr == other.arr && p == other.p;
            }
            bool operator!=(const Iterator& other) const {
                return !(*this == other);
            }

            T& operator*() const {
                return arr->at(p);
            }

            T* operator->() const {
                return &arr->at(p);
            }
        };

        MArray() = default;

        template<typename... Ts>
            requires(std::is_constructible_v<T, Ts> && ...)
        MArray(Ts&&... vals) {}

        void reserve(usize new_capacity) {
            if (new_capacity == 0)
                return;

            if (_capacity == 0) {
                _data = Allocator{}.allocate(new_capacity);
                _capacity = new_capacity;
                return;
            }

            const auto new_data = Allocator{}.allocate(new_capacity);

            const auto to_copy = min(_size, new_capacity);

            if constexpr (std::is_trivially_move_constructible_v<T>)
                memcpy(new_data, _data, to_copy * sizeof(T));
            else
                for (usize i = 0; i < to_copy; ++i)
                    new (new_data + i) T{std::move(_data[i])};

            Allocator{}.deallocate(_data, _capacity);
            _data = new_data;
            _capacity = new_capacity;
            _size = min(_size, _capacity);
        }

        void resize(usize new_size) {
            reserve(new_size);

            if (_size < _capacity) {
                if constexpr (std::is_trivially_constructible_v<T, void>)
                    memset(_data + _size, 0, (_capacity - _size) * sizeof(T));
                else {
                    const auto count = _capacity - _size;
                    for (usize i = 0; i < count; ++i)
                        new (_data + _size + i) T{};
                }
            }
        }

      private:
        void emplace_helper(usize count = 1) {
            if (_capacity == 0)
                reserve(max(8, count));
            else if (_size + count > _capacity)
                reserve(max(_capacity + _capacity / 2, _size + count));
        }

      public:
        template<typename... Args>
        T& emplace_back(Args&&... args) {
            emplace_helper();

            new (_data + _size++) T{std::forward<Args>(args)...};
        }

        template<typename Arg>
            requires std::is_trivially_copyable_v<T>
        T& emplace_back(const Arg& arg) {
            emplace_helper();

            _data[_size++] = arg;
        }

        T pop_back() {
            MAssert("Popping from empty array")(_size > 0);
            return std::move(_data[--_size]);
        }

        T pop_at(const Iterator& it) {
            const auto loc = it.p;

            MAssert("Popping from empty array")(_size > 0);
            T temp = std::move(_data[loc]);

            if constexpr (std::is_trivially_copyable_v<T>)
                memmove(_data + loc, _data + loc + 1, _size - (loc + 1));
            else
                for (usize i = loc; i < _size - 1; ++i)
                    std::swap(_data[i], _data[i + 1]);

            --_size;

            return temp;
        }

        void erase(const Iterator& it) {
            std::ignore = pop_at(it);
        }

        template<typename... Args>
        T& emplace_at(const Iterator& it, Args&&... args) {
            const auto loc = it.p;

            if (loc == _size)
                return emplace_back(std::forward<Args>(args)...);

            emplace_helper();

            for (usize i = _size; i > loc; --i)
                std::swap(at(i), at(i - 1));

            new (&at(loc)) T{std::forward<Args>(args)...};
        }

        template<typename It>
            requires std::is_convertible_v<typename It::value_type, T>
        void insert(const Iterator& it, const It& items_begin, const It& items_end) {
            auto loc = it.p;
            const auto count = std::distance(items_begin, items_end);

            emplace_helper(count);

            if (loc == _size) {
                for (auto o = items_begin; o != items_end; ++o)
                    new (_data + _size++) T{*o};

                return;
            }

            if constexpr (std::is_trivially_copyable_v<T>)
                memmove(_data + loc + count, _data + loc, count * sizeof(T));
            else {
                const auto size_diff = _size - loc;
                for (usize i = _size + count - 1; i >= loc + count; --i)
                    new (_data + i) T{std::move(_data[i - size_diff])};
            }

            for (auto o = items_begin; o != items_end; ++o)
                new (_data + loc++) T{*o};

            _size += count;
        }

        void remove(const Iterator& from, const Iterator& to) {
            MAssert{"Iterators to remove must be from [ begin() ] to [ end() - 1 ]"}(from != end() && to != end());

            const auto count = std::distance(from, to);

            if constexpr (std::is_trivially_destructible_v<T> && std::is_trivially_copyable_v<T>)
                memmove(_data + from.p, _data + to.p, count * sizeof(T));
            else {
                for (usize i = from.p; i < _size - count; ++i) {
                    if (i <= to.p)
                        _data[i].~T();

                    new (_data + i) T{std::move(_data[i + count])};
                }
            }
            _size -= count;
        }

        T& at(usize loc) {
            MAssert("Index out of range in array")(loc < _size);
            return _data[loc];
        }
        const T& at(usize loc) const {
            MAssert("Index out of range in array")(loc < _size);
            return _data[loc];
        }

        T& operator[](usize i) {
            return at(i);
        }
        const T& operator[](usize i) const {
            return at(i);
        }

        auto begin() {
            return _data;
        }
        auto end() {
            return _data + _size;
        }

        auto begin() const {
            return const_cast<const T*>(_data);
        }
        auto end() const {
            return const_cast<const T*>(_data + _size);
        }

        T& front() {
            MAssert("Indexing empty array")(_size > 0);
            return _data[0];
        }
        const T& front() const {
            MAssert("Indexing empty array")(_size > 0);
            return _data[0];
        }

        T& back() {
            MAssert("Indexing empty array")(_size > 0);
            return _data[_size - 1];
        }
        const T& back() const {
            MAssert("Indexing empty array")(_size > 0);
            return _data[_size - 1];
        }


        auto data() const { return _data; }

        auto capacity() const { return _capacity; }

        auto size() const { return _size; }
    };
} // namespace mgm
