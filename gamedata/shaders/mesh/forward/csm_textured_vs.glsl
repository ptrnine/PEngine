const int MAX_POINT_LIGHTS = 16;
const int MAX_SPOT_LIGHTS  = 16;
const int CSM_MAPS_COUNT = 3;

uniform mat4 M;
uniform mat4 MVP;
uniform mat4 light_MVP[CSM_MAPS_COUNT];
uniform mat4 spot_MVP[MAX_SPOT_LIGHTS];

in  vec3  position_ms;
in  vec2  in_uv;
in  vec3  normal_ms;
in  vec3  tangent_ms;
in  vec3  bitangent_ms;
in  ivec4 bone_ids;
in  vec4  weights;

out vec2  uv;
out vec3  position_ws;
out vec3  normal_cs;
out vec3  tangent_ws;
out vec3  bitangent_ws;
out vec4  position_ls[CSM_MAPS_COUNT];
out vec4  spot_pos_ls[MAX_SPOT_LIGHTS];
out float z_pos_cs;

void main() {
    vec4 pos = vec4(position_ms, 1.0);
    gl_Position  = MVP * pos;
    uv           = in_uv;
    position_ws  = (M * pos).xyz;
    normal_cs    = (M * vec4(normal_ms, 0.0)).xyz;
    tangent_ws   = (M * vec4(tangent_ms, 0.0)).xyz;
    bitangent_ws = (M * vec4(bitangent_ms, 0.0)).xyz;

    for (int i = 0; i < CSM_MAPS_COUNT; ++i)
        position_ls[i] = light_MVP[i] * pos;

    for (int i = 0; i < MAX_SPOT_LIGHTS; ++i)
        spot_pos_ls[i] = spot_MVP[i] * pos;

    z_pos_cs = -gl_Position.z;
}

