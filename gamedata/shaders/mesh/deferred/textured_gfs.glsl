in vec2 uv;
in vec3 position_ws;
in vec3 normal_ws;
in vec3 tangent_ws;
in vec3 bitangent_ws;

layout (location = 0) out vec4 albedo;
layout (location = 1) out vec4 normal;
layout (location = 2) out vec4 position_depth;

uniform sampler2D texture0;
uniform sampler2D texture1;

vec3 calc_bumped_normal() {
    vec3 normal          = normalize(normal_ws);
    vec3 tangent         = normalize(tangent_ws);
    vec3 bitangent       = normalize(bitangent_ws);
    vec3 bump_map_normal = 2.0 * texture(texture1, uv).xyz - vec3(1.0, 1.0, 1.0);
    mat3 TBN             = mat3(tangent, bitangent, normal);
    vec3 new_normal      = TBN * bump_map_normal;
    return normalize(new_normal);
}

void main() {
    albedo         = vec4(texture(texture0, uv).rgb, 1.0);
    normal         = vec4(calc_bumped_normal(), 0.0);
    position_depth = vec4(position_ws, 1/gl_FragCoord.w);
}
