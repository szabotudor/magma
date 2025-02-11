#pragma once
#include "mgmath.hpp"
#include <string>
#include <typeinfo>
#include <unordered_map>


namespace mgm {
    struct GPUSettings {
        enum class StateAttribute {
            // Clear data
            CLEAR,

            // Depth testing data
            DEPTH,

            // Face culling data
            CULLING,

            // Blending data
            BLENDING,

            // Viewport to render to
            VIEWPORT,

            // Scissor box
            SCISSOR
        };

        struct Clear {
            mgm::vec4f color{0.1f, 0.2f, 0.3f, 1.0f};
            bool color_buffer = true,
                depth_buffer = true,
                stencil_buffer = true;
        } clear{};
        
        struct Depth {
            bool enabled = false;
        } depth_testing{};

        struct Culling {
            enum class Type {
                // Don't cull any faces
                NO_CULLING,

                // Cull (skip drawing) faces with vertices going clockwise (usually the back faces)
                CLOCKWISE,

                // Cull (skip drawing) faces with vertices going counterclockwise (usually the front face)
                COUNTERCLOCKWISE
            } type = GPUSettings::Culling::Type::NO_CULLING;
        } culling{};

        struct Blending {
            bool enabled = false;
            enum class Factor {
                ZERO, ONE,
                SRC_COLOR, ONE_MINUS_SRC_COLOR,
                DST_COLOR, ONE_MINUS_DST_COLOR,
                SRC_ALPHA, ONE_MINUS_SRC_ALPHA,
                DST_ALPHA, ONE_MINUS_DST_ALPHA
            } src_color_factor = Factor::ONE, dst_color_factor = Factor::ZERO,
              src_alpha_factor = Factor::ONE, dst_alpha_factor = Factor::ZERO;
            enum class Equation {
                ADD, SRC_MINUS_DST, DST_MINUS_SRC, MIN, MAX
            } color_equation = Equation::ADD, alpha_equation = Equation::ADD;
        } blending{};

        struct Viewport {
            vec2i32 top_left{}, bottom_right{};
        } viewport{};

        struct Scissor {
            vec2i32 top_left{}, bottom_right{};
            bool enabled = false;
        } scissor{};
    };

    struct BufferCreateInfo {
        enum class Type {
            INVALID, RAW, INDEX
        };
        private:
        Type usage_type = Type::INVALID;
        void* raw_data = nullptr;
        size_t data_type{};
        size_t buffer_size = 0;
        size_t data_point_size_bytes = 0;

        public:
        BufferCreateInfo() = default;

        template<typename T>
        BufferCreateInfo(Type type, T* data, size_t size)
            : usage_type{type}, raw_data{(void*)data}, data_type{typeid(T).hash_code()}, buffer_size{size}, data_point_size_bytes{sizeof(T)} {}

        Type type() const { return usage_type; }
        void* data() const { return raw_data; }
        size_t size() const { return buffer_size; }
        size_t type_id_hash() const { return data_type; }
        size_t data_point_size() const { return data_point_size_bytes; }
    };

    struct ShaderCreateInfo {
        enum class Type {
            GRAPHICS, COMPUTE
        };
        struct SingleShaderInfo {
            enum class Type {
                VERTEX, PIXEL, COMPUTE
            } type = Type::VERTEX;
            std::string source{};
        };
        std::unordered_map<std::string, std::string> shader_sources{};
    };

    struct TextureCreateInfo {
        std::string name{};
        int32_t num_channels = 4;
        int32_t channel_size_in_bytes = 1;
        int32_t dimmentions = 2;
        vec2i32 size{};
        void* data = nullptr;
    };
}
