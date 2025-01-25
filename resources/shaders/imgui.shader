parameter mat4 Proj

texture(2D) Texture

func vec3 vertex(vec3 Vert, vec4 VertColor, vec2 TexCoords) {
    Frag_TexCoords = TexCoords
    Frag_VertColor = VertColor
    return Proj * vec4(Vert, 1.0f)
}

func vec4 pixel(vec2 Frag_TexCoords, vec4 Frag_VertColor) {
    return Frag_VertColor * Texture[Frag_TexCoords]
}
