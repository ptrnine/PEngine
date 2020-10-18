                     in vec3 position_ms;
layout(location = 5) in mat4 MVP;

void main() {
    gl_Position = MVP * vec4(position_ms, 1.0);
}

