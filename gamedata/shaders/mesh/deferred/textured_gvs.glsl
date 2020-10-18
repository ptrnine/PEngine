uniform mat4 M;
uniform mat4 MVP;

in vec3 position_ms;
in vec2 in_uv;
in vec3 normal_ms;
in vec3 tangent_ms;
in vec3 bitangent_ms;

out vec2 uv;
out vec3 position_ws;
out vec3 normal_ws;
out vec3 tangent_ws;
out vec3 bitangent_ws;

void main() {
    vec4 pos = vec4(position_ms, 1.0);
    gl_Position  = MVP * pos;
    uv           = in_uv;
    position_ws  = (M * pos).xyz;
    normal_ws    = (M * vec4(normal_ms, 0.0)).xyz;
    tangent_ws   = (M * vec4(tangent_ms, 0.0)).xyz;
    bitangent_ws = (M * vec4(bitangent_ms, 0.0)).xyz;
}
