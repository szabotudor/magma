#include "built-in_components/renderable.hpp"


namespace mgm {
    Transform::Transform(const SerializedData<Transform>& json) {
        pos = deserialize(SerializedData<vec3f>(json["position"]));
        scale = deserialize(SerializedData<vec3f>(json["scale"]));
        rot = deserialize(SerializedData<vec4f>(json["rotation"]));
    }

    Transform::operator SerializedData<Transform>() {
        SerializedData<Transform> res{};
        res["position"] = serialize(pos);
        res["scale"] = serialize(scale);
        res["rotation"] = serialize(rot);
        return res;
    }
}
