#include "RayTracer.h"
#include <limits>
#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.1415926535
#endif

RayTracer::RayTracer(int w, int h, int depth)
    : width(w), height(h), maxDepth(depth), framebuffer(w * h) {}

const std::vector<Vec3>& RayTracer::getFramebuffer() const {
    return framebuffer;
}

void RayTracer::render(const Scene& scene) {
    Vec3 forward = (scene.camera.lookat - scene.camera.position).normalized();
    Vec3 right = forward.cross(Vec3(0, 1, 0)).normalized();
    Vec3 up = right.cross(forward).normalized();

    float fov = 90.0f;
    float aspect = float(width) / height;
    float scale = tanf(fov * 0.5f * M_PI / 180.f);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float u = (2 * ((x + 0.5f) / width) - 1) * aspect * scale;
            float v = (1 - 2 * ((y + 0.5f) / height)) * scale;

            Vec3 dir = (forward + right * u + up * v).normalized();
            Ray ray(scene.camera.position, dir);

            Vec3 color = trace(ray, scene, maxDepth);
            framebuffer[y * width + x] = color;
        }
    }
}

Vec3 RayTracer::trace(const Ray& ray, const Scene& scene, int depth) {
    if (depth <= 0) return Vec3(0, 0, 0);

    Vec3 hit, normal;
    Material mat;
    if (!intersect(ray, scene, hit, normal, mat))
        return Vec3(0.1f, 0.1f, 0.1f); // Background

    Vec3 color = mat.diffuse_color * 0.1f; // Ambient

    for (const auto& light : scene.lights) {
        Vec3 toLight = (light.position - hit).normalized();

        // Shadow ray
        Ray shadowRay(hit + normal * 0.001f, toLight);
        Vec3 shadowHit, shadowNormal;
        Material tmp;
        if (!intersect(shadowRay, scene, shadowHit, shadowNormal, tmp)) {
            float diff = std::max(normal.dot(toLight), 0.0f);
            color += mat.diffuse_color * light.color * diff;
        }
    }

    if (mat.reflectivity > 0.0f) {
        Vec3 reflectDir = ray.direction - normal * 2.f * ray.direction.dot(normal);
        Ray reflectRay(hit + normal * 0.001f, reflectDir);
        color = color * (1.0f - mat.reflectivity) + trace(reflectRay, scene, depth - 1) * mat.reflectivity;
    }

    return color;
}

bool RayTracer::intersect(const Ray& ray, const Scene& scene, Vec3& hit, Vec3& normal, Material& mat) {
    float tMin = std::numeric_limits<float>::max();
    bool found = false;

    for (const auto& s : scene.spheres) {
        float t;
        Vec3 hp, n;
        if (intersectSphere(ray, s, t, hp, n) && t < tMin) {
            tMin = t;
            hit = hp;
            normal = n;
            mat = s.material;
            found = true;
        }
    }

    for (const auto& p : scene.planes) {
        float t;
        Vec3 hp, n;
        if (intersectPlane(ray, p, t, hp, n) && t < tMin) {
            tMin = t;
            hit = hp;
            normal = n;
            mat = p.material;
            found = true;
        }
    }

    return found;
}

bool RayTracer::intersectSphere(const Ray& ray, const Sphere& s, float& t, Vec3& hit, Vec3& normal) {
    Vec3 oc = ray.origin - s.center;
    float b = 2.0f * oc.dot(ray.direction);
    float c = oc.dot(oc) - s.radius * s.radius;
    float disc = b * b - 4 * c;

    if (disc < 0) return false;

    float sqrtDisc = sqrtf(disc);
    float t0 = (-b - sqrtDisc) * 0.5f;
    float t1 = (-b + sqrtDisc) * 0.5f;
    t = (t0 > 0) ? t0 : t1;

    if (t <= 0) return false;

    hit = ray.origin + ray.direction * t;
    normal = (hit - s.center).normalized();
    return true;
}

bool RayTracer::intersectPlane(const Ray& ray, const Plane& p, float& t, Vec3& hit, Vec3& normalOut) {
    float denom = p.normal.dot(ray.direction);
    if (fabs(denom) < 1e-6) return false;

    float tHit = (p.point - ray.origin).dot(p.normal) / denom;
    if (tHit < 0) return false;

    t = tHit;
    hit = ray.origin + ray.direction * t;
    normalOut = p.normal;
    return true;
}