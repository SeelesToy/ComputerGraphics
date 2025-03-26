#pragma once
#include "do_not_edit.h"

glm::vec3 DoNothing(triangle* tri, int depth, glm::vec3 p, glm::vec3 dir) {
    return glm::vec3(0);
}

glm::vec3 Shade(triangle* tri, int depth, glm::vec3 p, glm::vec3 dir) {
    glm::vec3 surface_color = tri->v1.col; // 使用顶点颜色
    glm::vec3 normal = glm::normalize(tri->v1.nor); // 使用顶点法线
    glm::vec3 col(0.1f * surface_color); // 环境光贡献

    // 计算光源方向
    glm::vec3 lightDir = glm::normalize(light_pos - p);
    float distance_to_light = glm::distance(light_pos, p);

    // 阴影测试
    float shadow_t = FLT_MAX;
    glm::vec3 shadow_col(0);
    trace(p + normal * 0.001f, lightDir, shadow_t, shadow_col, depth + 1, DoNothing);

    if (shadow_t >= distance_to_light - 0.001f) { // 无遮挡
        float diff = max(glm::dot(normal, lightDir), 0.0f);
        col += diff * surface_color;
    }

    // 反射计算
    if (tri->reflect && depth < max_recursion_depth) {
        glm::vec3 reflectDir = glm::reflect(dir, normal);
        glm::vec3 reflect_col(0);
        float reflect_t;
        trace(p + normal * 0.001f, reflectDir, reflect_t, reflect_col, depth + 1, Shade);
        col += reflect_col;
    }

    return col;
}

bool PointInTriangle(glm::vec3 pt, glm::vec3 v1, glm::vec3 v2, glm::vec3 v3) {
    glm::vec3 e1 = v2 - v1;
    glm::vec3 e2 = v3 - v1;
    glm::vec3 n = glm::cross(e1, e2);

    glm::vec3 c0 = glm::cross(e1, pt - v1);
    if (glm::dot(c0, n) < 0) return false;

    glm::vec3 e3 = v3 - v2;
    glm::vec3 c1 = glm::cross(e3, pt - v2);
    if (glm::dot(c1, n) < 0) return false;

    glm::vec3 e4 = v1 - v3;
    glm::vec3 c2 = glm::cross(e4, pt - v3);
    if (glm::dot(c2, n) < 0) return false;

    return true;
}

float RayTriangleIntersection(glm::vec3 o, glm::vec3 dir, triangle* tri, glm::vec3& point) {
    glm::vec3 v1 = tri->v1.pos;
    glm::vec3 v2 = tri->v2.pos;
    glm::vec3 v3 = tri->v3.pos;

    glm::vec3 e1 = v2 - v1;
    glm::vec3 e2 = v3 - v1;
    glm::vec3 n = glm::cross(e1, e2);

    float len = glm::length(n);
    if (len < 1e-8f) return FLT_MAX; // 退化三角形
    n = glm::normalize(n);

    float denom = glm::dot(dir, n);
    if (fabs(denom) < 1e-6f) return FLT_MAX;

    float t = glm::dot(v1 - o, n) / denom;
    if (t < 0.001f) return FLT_MAX; // 避免自相交

    point = o + dir * t;

    if (PointInTriangle(point, v1, v2, v3)) {
        return t;
    }
    return FLT_MAX;
}

void trace(glm::vec3 o, glm::vec3 dir, float& t, glm::vec3& io_col, int depth, closest_hit p_hit) {
    t = FLT_MAX;
    triangle* closest_tri = nullptr;
    glm::vec3 closest_point;

    for (auto& tri : tris) {
        glm::vec3 point;
        float current_t = RayTriangleIntersection(o, dir, &tri, point);
        if (current_t < t) {
            t = current_t;
            closest_tri = &tri;
            closest_point = point;
        }
    }

    if (t < FLT_MAX) {
        io_col += p_hit(closest_tri, depth, closest_point, dir);
    }
    else {
        io_col += bkgd;
    }
}

glm::vec3 GetRayDirection(float px, float py, int W, int H, float aspect_ratio, float fov) {
    float f = tan(fov / 2.0f);
    float x = (2.0f * (px + 0.5f) / W - 1.0f) * aspect_ratio * f;
    float y = (2.0f * (py + 0.5f) / H - 1.0f) * f;

    glm::vec3 d = x * glm::vec3(1, 0, 0) + y * glm::vec3(0, -1, 0) + glm::vec3(0, 0, -1);
    return glm::normalize(d);
}

void raytrace() {
    float aspect_ratio = static_cast<float>(PIXEL_W) / PIXEL_H;
    float fov = glm::radians(90.0f);

    for (int pixel_y = 0; pixel_y < PIXEL_H; ++pixel_y) {
        float percf = static_cast<float>(pixel_y) / PIXEL_H;
        int perci = static_cast<int>(percf * 100);
        std::clog << "\rScanlines done: " << perci << "%" << ' ' << std::flush;

        for (int pixel_x = 0; pixel_x < PIXEL_W; ++pixel_x) {
            glm::vec3 dir = GetRayDirection(pixel_x, pixel_y, PIXEL_W, PIXEL_H, aspect_ratio, fov);
            glm::vec3 col(0);
            float t;
            trace(eye, dir, t, col, 0, Shade);
            writeCol(col, pixel_x, pixel_y);
        }
    }
    std::clog << "\rFinish rendering.           \n";
}