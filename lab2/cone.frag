#version 450

layout(location = 0) in vec3 world_normal;
layout(location = 1) in vec3 world_position;

layout(location = 0) out vec4 out_color;

layout(push_constant) uniform PushConstants {
    float camera_x;
    float camera_y;
    float camera_z;
    float camera_yaw;
    float camera_pitch;
    float fov;

    float light_x;
    float light_y;
    float light_z;
    float light_r;
    float light_g;
    float light_b;
    float light_intensity;
    float ambient;

    float albedo_r;
    float albedo_g;
    float albedo_b;

    float spec_r;
    float spec_g;
    float spec_b;

    float shininess;
    float rotation_y;
    float aspect;
} pc;

void main()
{
    vec3 N = normalize(world_normal);

    vec3 light_vec = vec3(pc.light_x, pc.light_y, pc.light_z);
    vec3 L = length(light_vec) > 0.0001 ? normalize(-light_vec) : vec3(0.0, 1.0, 0.0);

    vec3 V = normalize(
        vec3(pc.camera_x, pc.camera_y, pc.camera_z)
        - world_position
    );

    vec3 H = normalize(L + V);

    float diffuse = max(dot(N, L), 0.0);
    float specular_factor = 0.0;

    if (diffuse > 0.0) {
        specular_factor = pow(
            max(dot(N, H), 0.0),
            max(pc.shininess, 1.0)
        );
    }

    vec3 albedo = vec3(
        pc.albedo_r,
        pc.albedo_g,
        pc.albedo_b
    );

    vec3 light_color = vec3(
        pc.light_r,
        pc.light_g,
        pc.light_b
    );

    vec3 specular_color = vec3(
        pc.spec_r,
        pc.spec_g,
        pc.spec_b
    );

    vec3 ambient_color = albedo * pc.ambient;

    vec3 diffuse_color =
        albedo * light_color
        * pc.light_intensity
        * diffuse;

    vec3 specular =
        specular_color
        * light_color
        * pc.light_intensity
        * specular_factor;

    vec3 color = ambient_color + diffuse_color + specular;

    out_color = vec4(clamp(color, 0.0, 1.0), 1.0);
}