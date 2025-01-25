#pragma once
#include "ecs.hpp"
#include "file.hpp"
#include "mgmath.hpp"
#include "systems/resources.hpp"
#include "mgmgpu.hpp"


namespace mgm {
    template<size_t S, typename T>
    vec<S, T> deserialize(const SerializedData<vec<S, T>>& data) {
        vec<S, T> res{};

        res.x() = T(data["x"]);

        if constexpr (S >= 2)
            res.y() = T(data["y"]);
        if constexpr (S >= 3)
            res.z() = T(data["z"]);
        if constexpr (S >= 4)
            res.w() = T(data["w"]);

        return res;
    }
    template<size_t S, typename T>
    SerializedData<vec<S, T>> serialize(const vec<S, T>& v) {
        SerializedData<vec<S, T>> res{};

        res["x"] = v.x();

        if constexpr (S >= 2)
            res["y"] = v.y();
        if constexpr (S >= 3)
            res["z"] = v.z();
        if constexpr (S >= 4)
            res["w"] = v.w();

        return res;
    }


    struct Transform {
        vec3f pos{};
        vec3f scale{};
        vec4f rot{};

        Transform() = default;

        Transform(const SerializedData<Transform>& json);
        operator SerializedData<Transform>();
    };


    class Shader : public Resource {
        public:
        MgmGPU::ShaderHandle created_shader = MgmGPU::INVALID_SHADER;

        Path loading_path{};

        Shader() = default;

        bool load_from_text(const std::string& source) override;

        bool load_from_file(const Path& main_source_file_path) override;

        ~Shader();
    };


    class Mesh : public Resource {
        ResourceReference<Shader> shader{};
        MgmGPU::DrawCall draw_call{};
        std::vector<MgmGPU::BufferHandle> leftover_buffers{};

        public:

        Mesh() = default;
        
        bool load_from_text(const std::string& obj) override;

        ~Mesh();
    };
}
