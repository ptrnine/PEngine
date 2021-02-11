in vec3  position_ms;
in vec2  in_uv;
in vec3  normal_ms;
in vec3  tangent_ms;
in vec3  bitangent_ms;
in ivec4 bone_ids;
in vec4  weights;
layout(location = 7) in mat4 MVP;
layout(location = 11) in mat4 M;

layout(std430, binding = 0) buffer bone_matrices_buf {
    mat4 bone_matrices[];
};

uniform int bones_count;

out vec2 uv;
out vec3 position_ws;
out vec3 normal_ws;
out vec3 tangent_ws;
out vec3 bitangent_ws;

void main() {
    mat4 bone_transform = bone_matrices[gl_InstanceID * bones_count + bone_ids[0]] * weights[0];
    bone_transform     += bone_matrices[gl_InstanceID * bones_count + bone_ids[1]] * weights[1];
    bone_transform     += bone_matrices[gl_InstanceID * bones_count + bone_ids[2]] * weights[2];
    bone_transform     += bone_matrices[gl_InstanceID * bones_count + bone_ids[3]] * weights[3];

    vec4 pos = (bone_transform * vec4(position_ms, 1.0));
    gl_Position  = MVP * pos;
    uv           = in_uv;
    position_ws  = (M * pos).xyz;
    normal_ws    = (M * (bone_transform * vec4(normal_ms, 0.0))).xyz;
    tangent_ws   = (M * (bone_transform * vec4(tangent_ms, 0.0))).xyz;
    bitangent_ws = (M * (bone_transform * vec4(bitangent_ms, 0.0))).xyz;
}
