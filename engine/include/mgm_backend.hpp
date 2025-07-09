#pragma once


#include "mgm_types.hpp"
namespace mgm {
    class MgmGraphics {
        struct Data;
        Data* data = nullptr;

      public:
#define HANDLE_CLASS(name)                                           \
    class name##Handle : public MId {                                \
      public:                                                        \
        using MId::MId;                                              \
        name##Handle(MId::_uint _base = static_cast<MId::_uint>(-1)) \
            : MId(_base) {}                                          \
    }

        HANDLE_CLASS(Buffer);
        HANDLE_CLASS(Shader);
        HANDLE_CLASS(Texture);

      private:
      public:
        MgmGraphics();
    };
} // namespace mgm
