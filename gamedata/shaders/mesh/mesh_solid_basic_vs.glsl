uniform mat4 MVP;
in vec3 position_ms;

void main() {
    gl_Position = MVP * vec4(position_ms, 1.0);
}

