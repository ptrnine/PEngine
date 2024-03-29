const int MAX_BONES = 128;

uniform mat4 MVP;
uniform mat4 bone_matrices[MAX_BONES];

                     in vec3  position_ms;
layout(location = 5) in ivec4 bone_ids;
layout(location = 6) in vec4  weights;

void main() {
    mat4 bone_transform = bone_matrices[bone_ids[0]] * weights[0];
    bone_transform     += bone_matrices[bone_ids[1]] * weights[1];
    bone_transform     += bone_matrices[bone_ids[2]] * weights[2];
    bone_transform     += bone_matrices[bone_ids[3]] * weights[3];
    gl_Position = MVP * (bone_transform * vec4(position_ms, 1.0));
}

