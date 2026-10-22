#version 450

const float PI = 3.14159265358979323846;
const int SEGMENTS = 50;

layout(location = 0) out vec3 world_normal;
layout(location = 1) out vec3 world_position;

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
    int triangle = gl_VertexIndex / 3;
    int vertex = gl_VertexIndex % 3;

    const float radius = 0.7;
    const float bottom_y = -0.7;
    const float apex_y = 0.7;
    const float height = apex_y - bottom_y;

    vec3 p;
    vec3 n;

    if (triangle < SEGMENTS) {
        float a0 = 2.0 * PI * float(triangle) / float(SEGMENTS);
        float a1 = 2.0 * PI * float(triangle + 1) / float(SEGMENTS);
        float am = 0.5 * (a0 + a1);

        vec3 apex = vec3(0.0, apex_y, 0.0);
        vec3 p0 = vec3(radius * cos(a0), bottom_y, radius * sin(a0));
        vec3 p1 = vec3(radius * cos(a1), bottom_y, radius * sin(a1));

        if (vertex == 0) {
            p = apex;
            n = normalize(vec3(cos(am), radius / height, sin(am)));
        } else if (vertex == 1) {
            p = p0;
            n = normalize(vec3(cos(a0), radius / height, sin(a0)));
        } else {
            p = p1;
            n = normalize(vec3(cos(a1), radius / height, sin(a1)));
        }
    } else {
        int i = triangle - SEGMENTS;

        float a0 = 2.0 * PI * float(i) / float(SEGMENTS);
        float a1 = 2.0 * PI * float(i + 1) / float(SEGMENTS);

        vec3 center = vec3(0.0, bottom_y, 0.0);
        vec3 p0 = vec3(radius * cos(a0), bottom_y, radius * sin(a0));
        vec3 p1 = vec3(radius * cos(a1), bottom_y, radius * sin(a1));

        if (vertex == 0)
            p = center;
        else if (vertex == 1)
            p = p1;
        else
            p = p0;

        n = vec3(0.0, -1.0, 0.0);
    }

    float c = cos(pc.rotation_y);
    float s = sin(pc.rotation_y);

    vec3 wp = vec3(
        p.x * c + p.z * s,
        p.y,
        -p.x * s + p.z * c
    );

    vec3 wn = normalize(vec3(
        n.x * c + n.z * s,
        n.y,
        -n.x * s + n.z * c
    ));

    world_position = wp;
    world_normal = wn;

    vec3 v = wp - vec3(
        pc.camera_x,
        pc.camera_y,
        pc.camera_z
    );

    float cy = cos(pc.camera_yaw);
    float sy = sin(pc.camera_yaw);
    float cp = cos(pc.camera_pitch);
    float sp = sin(pc.camera_pitch);

    v = vec3(
        v.x * cy - v.z * sy,
        v.y,
        v.x * sy + v.z * cy
    );

    v = vec3(
        v.x,
        v.y * cp - v.z * sp,
        v.y * sp + v.z * cp
    );

    float f = 1.0 / tan(max(pc.fov, 0.1) * 0.5);
    float z_near = 0.1;
    float z_far = 100.0;

    float clip_x = v.x * f / max(pc.aspect, 0.001);
    float clip_y = -v.y * f;
    float clip_z = v.z * (z_far / (z_near - z_far)) - (z_near * z_far) / (z_far - z_near);
    float clip_w = -v.z;

    gl_Position = vec4(clip_x, clip_y, clip_z, clip_w);
}