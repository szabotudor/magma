struct Light {
    int type // 0=directional, 1=point, 2=spotlight
    vec3 pos
    vec3 dir
    float param // angle(when spotlight), radius(when directional)
    vec4 color
}


buffer(3) Light lights


parameter mat4 transform
parameter mat4 camera
parameter mat4 proj

parameter vec4 ambient_color
parameter float ambient_intensity


texture(2D, 0) main_texture


func vec3 vertex(vec3 verts, vec3 norms, vec2 tex_coords) {
    norm = norms
    tex = tex_coords

    return proj * camera * transform * vec4(verts, 1.0f)
}

func vec4 pixel(norm, tex) {
    var vec4 color = vec4(0.0f)

    color += ambient_color * ambient_intensity

    var vec3 normal = normalize(norm)
    var vec4 tex_color = uv(texture, tex)

    var vec3 cam_pos = camera[3].xyz

    var int i = 0
    while (i < size(lights)) {
        var Light light = lights[i]

        i += 1
    }

    return color * tex_color
}
