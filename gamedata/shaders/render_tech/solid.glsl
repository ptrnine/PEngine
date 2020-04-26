uniform mat4 MVP;

shader solid_vs(in vec3 position_ms) {
    gl_Position = MVP * vec4(position_ms, 1.0);
}

layout (location = 5) in mat4 mvp;

shader solid_instanced_vs(in vec3 position_ms) {
    gl_Position = mvp * vec4(position_ms, 1.0);
}

shader solid_fs(out vec3 color) {
    color = vec3(1.0, 1.0, 1.0);
}

program solid
{
    vs(410) = solid_vs();
    fs(410) = solid_fs();
};

program solid_instanced
{
    vs(410) = solid_instanced_vs();
    fs(410) = solid_fs();
};