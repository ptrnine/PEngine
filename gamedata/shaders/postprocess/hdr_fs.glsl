uniform float gamma;
uniform float exposure;
uniform sampler2D screen_quad_texture;

in  vec2 uv;
out vec4 color;

void main() {
    vec3 hdr = texture(screen_quad_texture, uv).rgb;
    vec3 mapped = vec3(1.0) - exp(-hdr * exposure);
    color = vec4(pow(mapped, 1.0 / vec3(gamma)), 1.0);
}

