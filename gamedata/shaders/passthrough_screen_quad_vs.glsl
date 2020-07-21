/*
 * Simple passthrough vertex shader for screen quads
 * No uv vbo required
 */

in  vec3 position_ms;
out vec2 uv;

void main() {
    gl_Position = vec4(position_ms, 1.0);
    uv = (position_ms.xy + vec2(1.0, 1.0)) * 0.5;
}

