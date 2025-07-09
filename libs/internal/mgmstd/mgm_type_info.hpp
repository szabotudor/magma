#pragma once
#include <cstddef>


namespace mgm {
    // WTF
    // And thank you to this stack overflow poster: https://stackoverflow.com/a/77093119/10978039
    template<auto Id>
    struct TIDCounter {
        struct TIDGenerator {
            template<auto>
            friend consteval auto is_defined(TIDCounter<Id>) { return true; }
        };
        template<auto>
        friend consteval auto is_defined(TIDCounter<Id>);

        template<typename Tag = TIDCounter<Id>, auto = is_defined<0>(Tag{})>
        static consteval auto exists(auto) { return true; }

        static consteval auto exists(...) {
            TIDGenerator();
            return false;
        }
    };

    template<typename T>
    struct TypeID {
        template<typename U = T, auto Id = size_t{}>
        static inline consteval auto type_id() {
            if constexpr (TIDCounter<Id>::exists(Id)) {
                return type_id<T, Id + 1>();
            }
            else {
                return Id;
            }
        }

        static inline constexpr size_t _id = type_id<T>();

        inline constexpr operator size_t() const { return _id; }
    };
} // namespace mgm
