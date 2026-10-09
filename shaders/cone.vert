#version 450

const float PI = 3.141592;
const int SEGMENTS = 50;

layout(location = 0) out vec3 normal;

layout(push_constant) uniform PushConstants
{
    float rotation_y;
} push;

void main()
{
    int triangle = gl_VertexIndex / 3;
    int vertex = gl_VertexIndex % 3;

    vec3 position;
    vec3 n;

    if (triangle < SEGMENTS)
    {
        float a0 = 2.0 * PI * float(triangle) / float(SEGMENTS);
        float a1 = 2.0 * PI * float(triangle + 1) / float(SEGMENTS);

        vec3 apex = vec3(0.0, 0.7, 0.0);
        vec3 p0 = vec3(0.7 * cos(a0), -0.7, 0.7 * sin(a0));
        vec3 p1 = vec3(0.7 * cos(a1), -0.7, 0.7 * sin(a1));

        if (vertex == 0)
        {
            position = apex;
        }
        else if (vertex == 1)
        {
            position = p0;
        }
        else
        {
            position = p1;
        }

        vec3 edge1 = p0 - apex;
        vec3 edge2 = p1 - apex;

        n = normalize(cross(edge2, edge1));
    }
    else
    {
        int i = triangle - SEGMENTS;

        float a0 = 2.0 * PI * float(i) / float(SEGMENTS);
        float a1 = 2.0 * PI * float(i + 1) / float(SEGMENTS);

        vec3 center = vec3(0.0, -0.7, 0.0);
        vec3 p0 = vec3(0.7 * cos(a0), -0.7, 0.7 * sin(a0));
        vec3 p1 = vec3(0.7 * cos(a1), -0.7, 0.7 * sin(a1));

        if (vertex == 0)
        {
            position = center;
        }
        else if (vertex == 1)
        {
            position = p1;
        }
        else
        {
            position = p0;
        }

        n = vec3(0.0, -1.0, 0.0);
    }

    normal = n;

    float angle_x = 0.45;
    float angle_z = 0.0;

    float cos_x = cos(angle_x);
    float sin_x = sin(angle_x);

    float cos_y = cos(push.rotation_y);
    float sin_y = sin(push.rotation_y);

    float cos_z = cos(angle_z);
    float sin_z = sin(angle_z);

    float x = position.x * cos_y + position.z * sin_y;
    float z = -position.x * sin_y + position.z * cos_y;

    float y = position.y * cos_x - z * sin_x;
    z = position.y * sin_x + z * cos_x;

    float rotated_x = x * cos_z - y * sin_z;
    float rotated_y = x * sin_z + y * cos_z;

    x = rotated_x;
    y = rotated_y;

    z += 3.0;

    float aspect = 1280.0 / 720.0;
    float f = 1.3;

    gl_Position = vec4(x * f / (z * aspect), y * f / z, (z - 0.1) / 10.0, 1.0);
}