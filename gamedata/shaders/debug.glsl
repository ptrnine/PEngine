uniform mat4 MVP;
uniform vec3 aabb_color;

shader aabb_debug_vs(in vec3 position_ms) {
    gl_Position = MVP * vec4(position_ms, 1.0);
}

shader aabb_debug_fs(out vec3 color) {
    color = aabb_color;
}

program aabb_debug {
    vs(410) = aabb_debug_vs();
    fs(410) = aabb_debug_fs();
};

