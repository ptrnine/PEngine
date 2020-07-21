in vec3 position_ms;
in (location = 2) mat4 mvp;

void main() {
    gl_Position = mvp * vec4(position_ms, 1.0);
}

