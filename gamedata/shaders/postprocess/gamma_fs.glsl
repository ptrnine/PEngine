uniform float gamma;
uniform sampler2D screen_quad_texture;

in  vec2 uv;
out vec4 color;

void main() {
    color = vec4(pow(texture(screen_quad_texture, uv).rgb, vec3(gamma)), 1.0);
}

