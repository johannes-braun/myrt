#pragma once


float sdSphere(vec3 p, float s)
{
    return length(p) - s;
}


float opSmoothUnion(float d1, float d2, float k) {
    float h = clamp(0.5 + 0.5 * (d2 - d1) / k, 0.0, 1.0);
    return mix(d2, d1, h) - k * h * (1.0 - h);
}

float map(vec3 p)
{
    return opSmoothUnion(sdSphere(p - vec3(0, 0.7, 0), 1.5), sdSphere(p - vec3(0, 3, 0), 0.5), 1.0);
}

vec3 nor(vec3 p)
{
    vec3 n = vec3(0.0);
    for (int i = DONT_OPTIMIZE_ZERO; i < 4; i++)
    {
        vec3 e = 0.5773 * (2.0 * vec3((((i + 3) >> 1) & 1), ((i >> 1) & 1), (i & 1)) - 1.0);
        n += e * map(p + 0.0005 * e).x;
    }
    return normalize(n);
}

hit_t march_sdf(vec3 ro, vec3 rd, float tmax, bool hits_only)
{
    float res = -1;
    float t = 1e-3f;
    for (int i = DONT_OPTIMIZE_ZERO; i < 128 && t < tmax; i++)
    {
        float h = abs(map(ro + rd * t));
        if (h < 1e-5f)
        {
            res = t;
            break;
        }
        t += h;
    }

    if (res < 0)
    {
        hit_t h;
        h.hits = false;
        return h;
    }

    hit_t hit;
    hit.hits = true;

    if (hits_only)
        return hit;

    hit.t = res;
    hit.position = ro + t * rd;
    hit.normal = nor(hit.position);
    hit.material.albedo_rgba_unorm = 0xffffffff;
    hit.material.ior = 1.5;
    hit.material.roughness = 0.1;
    hit.material.metallic = 1;
    hit.material.transmission = 0;
    return hit;
}