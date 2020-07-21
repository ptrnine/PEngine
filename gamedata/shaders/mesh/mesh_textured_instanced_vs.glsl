in  vec3 position_ms;
in  vec2 in_uv;
in  mat4 mvp;

out vec2 uv;

void main() {
    gl_Position = mvp * vec4(position_ms, 1.0);
    uv          = in_uv;
}

