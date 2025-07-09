#pragma once
#include "mgm_backend.hpp"
#include "mgm_string.hpp"
#include "mgmath.hpp"


namespace mgm {
    class MgmWindow {
      public:
        enum class Option {
            FULLSCREEN,
            ALLOW_RESIZE,
            VSYNC,

            _NUM_OPTIONS
        };

        void set_option_private(Option option, bool value);

      private:
        struct Data;
        Data* data = nullptr;

        bool option_states[static_cast<int32>(Option::_NUM_OPTIONS)]{};

      public:
        MgmWindow(const vec2u32& size, const MString& title = "Magma");

        void set_option(Option option, bool value) {
            option_states[static_cast<int32>(option)] = value;
            set_option_private(option, value);
        }

        inline bool get_option(Option option) const {
            return option_states[static_cast<int32>(option)];
        }

        void add_graphics_backend(MgmGraphics& backend);

        bool should_close() const;

        void update();

        ~MgmWindow();
    };
} // namespace mgm
