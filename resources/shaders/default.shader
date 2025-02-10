parameter mat4 transform
parameter mat4 camera
parameter mat4 proj


texture(2D) main_texture


func vec3 vertex(vec3 verts, vec3 vert_colors, vec3 norms, vec2 tex_coords) {
    norm = norms
    tex = tex_coords
    pixel_vert_colors = vert_colors

    return vec4(verts, 1.0f) * transform * camera * proj
}

func vec4 pixel(vec3 norm, vec3 pixel_vert_colors, vec2 tex) {
    return vec4(norm, 1.0f)
}
