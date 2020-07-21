uniform sampler2D screen_quad_texture;

in  vec2 uv;
out vec4 color;

void main() {
    color = texture(screen_quad_texture, uv);
}

