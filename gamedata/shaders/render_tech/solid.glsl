const int MAX_BONES = 128;

uniform mat4 MVP;
uniform mat4 bone_matrices[MAX_BONES];

shader solid_vs(in vec3 position_ms) {
    gl_Position = MVP * vec4(position_ms, 1.0);
}

shader solid_skeleton_vs(
    in vec3 position_ms,
    in vec2 uv,
    in ivec4 bone_ids,
    in vec4 weights
) {
    mat4 bone_transform = bone_matrices[bone_ids[0]] * weights[0];
    bone_transform     += bone_matrices[bone_ids[1]] * weights[1];
    bone_transform     += bone_matrices[bone_ids[2]] * weights[2];
    bone_transform     += bone_matrices[bone_ids[3]] * weights[3];

    gl_Position = MVP * (bone_transform * vec4(position_ms, 1.0));
}

layout (location = 2) in mat4 mvp;
shader solid_instanced_vs(in vec3 position_ms) {
    gl_Position = mvp * vec4(position_ms, 1.0);
}

shader solid_fs(out vec3 color) {
    color = vec3(1.0, 1.0, 1.0);
}

program solid {
    vs(410) = solid_vs();
    fs(410) = solid_fs();
};

program solid_skeleton {
    vs(410) = solid_skeleton_vs();
    fs(410) = solid_fs();
};

program solid_instanced {
    vs(410) = solid_instanced_vs();
    fs(410) = solid_fs();
};