struct light_base_s {
    vec3  color;
    float ambient_intensity;
    float diffuse_intensity;
};

struct dir_light_s {
    light_base_s base;
    vec3        direction;
};

struct point_light_s {
    light_base_s base;
    vec3         position;
    float        attenuation_constant;
    float        attenuation_linear;
    float        attenuation_quadratic;
};

struct spot_light_s {
    point_light_s base;
    vec3          direction;
    float         cutoff;
};

uniform vec3  eye_pos_ws;
uniform float specular_power;
uniform float specular_intensity;
uniform int   light_type = -1;

uniform dir_light_s   dir_light;
uniform point_light_s point_light;
uniform spot_light_s  spot_light;


vec4 calc_light_base(light_base_s light,
                     vec3         light_direction,
                     vec3         normal_ws,
                     vec3         position_ws,
                     float        shadow_factor
) {
    vec4  ambient_color  = vec4(light.color, 1.0) * light.ambient_intensity;
    float diffuse_factor = dot(normal_ws, -light_direction);
    vec4  diffuse_color  = vec4(0.0);
    vec4  specular_color = vec4(0.0);

    if (diffuse_factor > 0.0) {
        diffuse_color = vec4(light.color, 1.0) * light.diffuse_intensity * diffuse_factor;

        vec3  view_dir    = normalize(eye_pos_ws - position_ws);
        vec3  halfway_dir = normalize(view_dir - light_direction);
        float specular_factor = pow(max(dot(normal_ws, halfway_dir), 0.0), specular_power);
        if (specular_factor > 0)
            specular_color = vec4(light.color, 1.0) * specular_intensity * specular_factor;
    }

    return ambient_color + shadow_factor * (diffuse_color + specular_color);
}

vec4 calc_dir_light(vec3 normal_ws, vec3 position_ws, float shadow_factor) {
    return calc_light_base(dir_light.base, dir_light.direction, normal_ws, position_ws, shadow_factor);
}

vec4 calc_point_light(point_light_s light, vec3 normal_ws, vec3 position_ws, float shadow_factor) {
    vec3 light_direction = position_ws - light.position;
    float _distance      = length(light_direction);
    light_direction      = normalize(light_direction);

    vec4  color          = calc_light_base(light.base, light_direction, normal_ws, position_ws, shadow_factor);
    float attenuation    = light.attenuation_constant + light.attenuation_linear * _distance +
                                                        light.attenuation_quadratic * _distance * _distance;
    return color / attenuation;
}

vec4 calc_spot_light(spot_light_s light, vec3 normal_ws, vec3 position_ws, float shadow_factor) {
    vec3  light_to_pixel = normalize(position_ws - light.base.position);
    float spot_factor    = dot(light_to_pixel, light.direction);
    vec4 color = calc_point_light(light.base, normal_ws, position_ws, shadow_factor);
    return color * (1.0 - (1.0 - spot_factor) * 1.0/(1.0 - light.cutoff));
}


uniform sampler2D albedo;
uniform sampler2D position_depth;
uniform sampler2D normal;

out vec4 color;

const int SM_STEPS = 5;
const int SM_LOW   = -SM_STEPS/2;
const int SM_UP    = (SM_STEPS + 1)/2;
const float SM_DIV = SM_STEPS * SM_STEPS;

/*
float edge_detection() {
    vec2 texel_size = 1.0 / vec2(textureSize(albedo, 0));

    vec3 normals[int(SM_DIV)];
    vec3 normal_sum = vec3(0.0);

    int idx = 0;
    for (int i = SM_LOW; i < SM_UP; ++i) {
        for (int j = SM_LOW; j < SM_UP; ++j) {
            normals[idx] = texture(normal, uv + vec2(i, j) * texel_size).xyz;
            normal_sum += normals[idx++];
        }
    }

    normal_sum = normalize(normal_sum);
    float pts = 0.0;

    for (int i = 0; i < SM_DIV; ++i)
        pts += abs(dot(normal_sum, normals[i]));

    return 4.0 * (1.0 - pts / SM_DIV) > 0.2 ? 1.0 : 0.0;
}


vec3 smooth_lookup(vec3 c0) {
    float detect = edge_detection();
        vec2 texel_size = 1.0 / vec2(textureSize(albedo, 0));
        vec3 color = vec3(0.0);

        for (int i = SM_LOW; i < SM_UP; ++i)
            for (int j = SM_LOW; j < SM_UP; ++j)
                color += texture(albedo, uv + vec2(i, j) * texel_size).rgb;

        return detect * (color / SM_DIV) + (1.0 - detect) * c0;
}
*/

vec2 calc_uv() {
    return gl_FragCoord.xy / vec2(textureSize(albedo, 0));
}

void main() {
    vec2 uv  = calc_uv();
    vec4 pd  = texture(position_depth, uv);
    vec4 n   = texture(normal, uv);
    vec4 a   = texture(albedo, uv);

    vec3  position_ws = pd.xyz;
    vec3  normal_ws   = n.xyz;
    vec3  diffuse     = a.rgb;
    float depth       = pd.w;
    float emissive    = n.w;

    vec4 total_light = vec4(0.0);

    switch (light_type) {
        case 0:
            total_light += calc_dir_light(normal_ws, position_ws, 1.0);
            break;
        case 1:
            total_light += calc_point_light(point_light, normal_ws, position_ws, 1.0);
            break;
        case 2:
            total_light += calc_spot_light(spot_light, normal_ws, position_ws, 1.0);
            break;
    }

    color = emissive * vec4(diffuse, 1.0) + (vec4(diffuse, 1.0) * total_light * (1.0 - emissive));
}
