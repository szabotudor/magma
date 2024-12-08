#include "built-in_components/renderable.hpp"
#include "json.hpp"


namespace mgm {
    Transform::Transform(const JObject& json) {
        pos = to_vec3<float>(json["pos"]);
        scale = to_vec3<float>(json["scale"]);
        rot = to_vec4<float>(json["rot"]);
    }

    Transform::operator JObject() {
        JObject res{};
        res["pos"] = from_vec(pos);
        res["scale"] = from_vec(scale);
        res["rot"] = from_vec(rot);
        return res;
    }
}
