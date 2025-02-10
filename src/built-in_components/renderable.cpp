#include "built-in_components/renderable.hpp"
#include "backend_settings.hpp"
#include "engine.hpp"
#include "logging.hpp"
#include "mgmgpu.hpp"
#include "shaders.hpp"
#include <string>


#define TINYOBJLOADER_IMPLEMENTATION
#include "tools/tiny_obj_loader.h"


namespace mgm {
    Transform::Transform(const SerializedData<Transform>& json) {
        pos = deserialize(SerializedData<vec3f>(json["position"]));
        scale = deserialize(SerializedData<vec3f>(json["scale"]));
        rot = static_cast<quatf>(deserialize(SerializedData<vec4f>(json["rotation"])));
    }

    Transform::operator SerializedData<Transform>() {
        SerializedData<Transform> res{};
        res["position"] = serialize(pos);
        res["scale"] = serialize(scale);
        res["rotation"] = serialize(static_cast<vec4f>(rot));
        return res;
    }

    mat4f Transform::as_matrix() const {
        const mat4f translate_mat {
            1.0f, 0.0f, 0.0f, pos.x,
            0.0f, 1.0f, 0.0f, pos.y,
            0.0f, 0.0f, 1.0f, pos.z,
            0.0f, 0.0f, 0.0f, 1.0f
        };

        const mat4f rotation_mat = rot.as_rotation_mat4();

        const mat4f scale_mat {
            scale.x, 0.0f, 0.0f, 0.0f,
            0.0f, scale.y, 0.0f, 0.0f,
            0.0f, 0.0f, scale.z, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };

        return scale_mat * translate_mat * rotation_mat;
    }

    Transform Transform::inverse() const {
        Transform inv{};
        inv.rot = rot.inv();
        inv.scale = 1.0f / scale;
        inv.pos = inv.rot.rotate(-pos * inv.scale);
        return inv;
    }

    Transform Transform::operator*(const Transform& other) const {
        Transform res{};
        res.pos = pos + rot.rotate(other.pos * scale);
        res.scale = scale * other.scale;
        res.rot = rot * other.rot;
        return res;
    }

    Transform& Transform::operator*=(const Transform& other) {
        return *this = *this * other;
    }


    bool Shader::load_from_text(const std::string& source) {
        auto& gpu = MagmaEngine{}.graphics();

        MgmGPUShaderBuilder builder{};

        if (!loading_path.empty()) {
            builder.set_load_function([&](const std::string& path) -> std::string {
                const auto actual_path = loading_path / path;

                if (!MagmaEngine{}.file_io().exists(actual_path))
                    return "";

                return MagmaEngine{}.file_io().read_text(actual_path);
            });
        }

        builder.build(source);
        
        created_shader = gpu.create_shader(builder);

        return created_shader != MgmGPU::INVALID_SHADER;
    }
    
    bool Shader::load_from_file(const Path& main_source_file_path) {
        const auto source = MagmaEngine{}.file_io().read_text(main_source_file_path);
        loading_path = main_source_file_path.back();
        const auto success = load_from_text(source);
        loading_path = {};
        return success;
    }

    Shader::~Shader() {
        if (created_shader != MgmGPU::INVALID_SHADER)
            MagmaEngine{}.graphics().destroy_shader(created_shader);
    }


    bool Mesh::load_from_text(const std::string& obj) {
        tinyobj::ObjReader reader{};
        reader.ParseFromString(obj, "");

        const auto& attrib = reader.GetAttrib();
        const auto& shapes = reader.GetShapes();

        std::vector<vec3f> vertices{};
        
        std::vector<vec3f> vert_colors{};

        std::vector<vec3f> normals{};

        std::vector<vec2f> tex_coords{};

        for (const auto& shape : shapes) {
            size_t idx_offset = 0;
            for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
                size_t f_size = shape.mesh.num_face_vertices[f];

                if (f_size != 3) {
                    Logging{"OBJ Loader"}.error("Only 3-sided faces are supported (triangles)");
                    return false;
                }

                for (size_t v_idx = 0; v_idx < f_size; ++v_idx) {
                    const auto idx = shape.mesh.indices[idx_offset + v_idx];

                    vertices.emplace_back(
                        attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + 0],
                        attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + 1],
                        attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + 2]
                    );

                    if (!attrib.colors.empty())
                        vert_colors.emplace_back(
                            attrib.colors[3 * static_cast<size_t>(idx.vertex_index) + 0],
                            attrib.colors[3 * static_cast<size_t>(idx.vertex_index) + 1],
                            attrib.colors[3 * static_cast<size_t>(idx.vertex_index) + 2]
                        );

                    if (idx.normal_index >= 0)
                        normals.emplace_back(
                            attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 0],
                            attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 1],
                            attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 2]
                        );
                    
                    if (idx.texcoord_index >= 0)
                        tex_coords.emplace_back(
                            attrib.texcoords[2 * static_cast<size_t>(idx.texcoord_index) + 0],
                            attrib.texcoords[2 * static_cast<size_t>(idx.texcoord_index) + 1]
                        );
                }
                idx_offset += f_size;
            }
        }

        auto& gpu = MagmaEngine{}.graphics();

        std::unordered_map<std::string, MgmGPU::BufferHandle> buffer_names{};
        
        vertex_buffer = gpu.create_buffer(BufferCreateInfo{
            BufferCreateInfo::Type::RAW,
            vertices.data(),
            vertices.size()
        });
        buffer_names["verts"] = vertex_buffer;

        if (!vert_colors.empty()) {
            color_buffer = gpu.create_buffer(BufferCreateInfo{
                BufferCreateInfo::Type::RAW,
                vert_colors.data(),
                vert_colors.size()
            });
            buffer_names["vert_colors"] = color_buffer;
        }

        if (!normals.empty()) {
            normal_buffer = gpu.create_buffer(BufferCreateInfo{
                BufferCreateInfo::Type::RAW,
                normals.data(),
                normals.size()
            });
            buffer_names["norms"] = normal_buffer;
        }
        
        if (!tex_coords.empty()) {
            tex_coord_buffer = gpu.create_buffer(BufferCreateInfo{
                BufferCreateInfo::Type::RAW,
                tex_coords.data(),
                tex_coords.size()
            });
            buffer_names["tex_coords"] = tex_coord_buffer;
        }

        buffers_object = gpu.create_buffers_object(buffer_names);

        shader = MagmaEngine{}.resource_manager().get_or_load<Shader>("resources://shaders/default.shader");
        if (!shader.valid())
            return false;

        return true;
    }

    Mesh::~Mesh() {
        auto& gpu = MagmaEngine{}.graphics();

        gpu.destroy_buffers_object(buffers_object);

        gpu.destroy_buffer(vertex_buffer);
        gpu.destroy_buffer(normal_buffer);
        gpu.destroy_buffer(tex_coord_buffer);
    }
}
