const int MAX_BONES = 128;
const int MAX_SPOT_LIGHTS = 16;

uniform mat4 spot_MVP[MAX_SPOT_LIGHTS];
uniform int  spot_indices[MAX_SPOT_LIGHTS];
uniform mat4 bone_matrices[MAX_BONES];

                     in vec3  position_ms;
layout(location = 5) in ivec4 bone_ids;
layout(location = 6) in vec4  weights;

out vs_out {
    float    depth;
    flat int spot_idx;
} gs_input;

void main() {
    mat4 bone_transform = bone_matrices[bone_ids[0]] * weights[0];
    bone_transform     += bone_matrices[bone_ids[1]] * weights[1];
    bone_transform     += bone_matrices[bone_ids[2]] * weights[2];
    bone_transform     += bone_matrices[bone_ids[3]] * weights[3];
    gl_Position = spot_MVP[gl_InstanceID] * (bone_transform * vec4(position_ms, 1.0));

    gs_input.depth    = gl_Position.z;
    gs_input.spot_idx = spot_indices[gl_InstanceID];
}

