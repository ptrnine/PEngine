in  vec2 uv;
out vec4 color;

uniform sampler2D screen_quad_texture;
uniform float     time;
uniform float     noise_intensity;
uniform float     jitter_intensity;
uniform float     stripe_jitter;
uniform float     jump_intensity;

float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

vec3 noise(vec2 uv) {
    float r = rand(vec2(time*uv.x, time*uv.y));
    float g = rand(vec2(time*uv.y, time*uv.x));
    float b = rand(vec2(abs(time*(uv.x-uv.y)), time*uv.y));
    return vec3(r, g, b);
}

void main() {
    //vec2 uv = fragCoord/iResolution.xy;
    vec3 noise_col = noise(uv);

    float jitter_f = rand(vec2(time, uv.y));
    float y_jump = (rand(vec2(0, time)) - 0.5) * jump_intensity;
    vec2 uv_ = vec2(uv.x + jitter_f * jitter_intensity, uv.y + y_jump);
    vec4 texcolor = texture(screen_quad_texture, uv_);

    float white = rand(vec2(floor(uv_.y * 150.0), floor(uv_.x * 90.0)) +
                       vec2(time * 10.0, 0.0));

    float w_jitter = rand(vec2(time * uv_.x, uv_.y));
    float stripe_jit = (w_jitter - 0.5) * stripe_jitter;
    if (white > ((10.0 - 30.0 * uv_.y + stripe_jit)) ||
        white < ((1.4  - 5.5 * uv_.y) + stripe_jit)) {
        color = texcolor * (1.0 - noise_intensity) +
            vec4(noise_col, 1.0) * noise_intensity;
    } else {
        color = vec4(noise_col.x + 0.7,
                     noise_col.y + 0.7,
                     noise_col.z + 0.7, 1.0);
    }
}

