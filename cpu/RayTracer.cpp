#include "RayTracer.h"
#include <limits>
#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdlib>
#include <iostream>

#ifndef M_PI
#define M_PI 3.1415926535
#endif
void print_progress_bar(float progress);

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
        print_progress_bar(float(y + 1) / height);
    }
    std::cout << std::endl;
}

Vec3 RayTracer::trace(const Ray& ray, const Scene& scene, int depth) {
    if (depth <= 0) return Vec3(0, 0, 0);

    Vec3 hit, normal;
    Material mat;
    if (!intersect(ray, scene, hit, normal, mat))
        return Vec3(0.1f, 0.1f, 0.1f); // Background color

    Vec3 color = mat.diffuse_color * 0.1f; // Ambient term

    // emission
    color += mat.emission; 

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

    for (const auto& c : scene.cubes) {
        float t;
        Vec3 hp, n;
        if (intersectCube(ray, c, t, hp, n) && t < tMin) {
            tMin = t;
            hit = hp;
            normal = n;
            mat = c.material;
            found = true;
        }
    }
    for (const auto& tri : scene.triangles) {
        float t;
        Vec3 hp, n;
        if (intersectTriangle(ray, tri, t, hp, n) && t < tMin) {
            tMin = t;
            hit = hp;
            normal = n;
            mat = tri.material;
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

// Ray-AABB (cube) intersection
bool RayTracer::intersectCube(const Ray& ray, const Cube& cube, float& t, Vec3& hit, Vec3& normal) {
    float tmin = (cube.min.x - ray.origin.x) / ray.direction.x;
    float tmax = (cube.max.x - ray.origin.x) / ray.direction.x;
    if (tmin > tmax) std::swap(tmin, tmax);

    float tymin = (cube.min.y - ray.origin.y) / ray.direction.y;
    float tymax = (cube.max.y - ray.origin.y) / ray.direction.y;
    if (tymin > tymax) std::swap(tymin, tymax);

    if ((tmin > tymax) || (tymin > tmax))
        return false;

    if (tymin > tmin) tmin = tymin;
    if (tymax < tmax) tmax = tymax;

    float tzmin = (cube.min.z - ray.origin.z) / ray.direction.z;
    float tzmax = (cube.max.z - ray.origin.z) / ray.direction.z;
    if (tzmin > tzmax) std::swap(tzmin, tzmax);

    if ((tmin > tzmax) || (tzmin > tmax))
        return false;

    if (tzmin > tmin) tmin = tzmin;
    if (tzmax < tmax) tmax = tzmax;

    if (tmax < 0) return false;

    t = tmin > 0 ? tmin : tmax;
    if (t < 0) return false;

    hit = ray.origin + ray.direction * t;

    // Compute normal
    const float eps = 1e-4f;
    if (fabs(hit.x - cube.min.x) < eps) normal = Vec3(-1,0,0);
    else if (fabs(hit.x - cube.max.x) < eps) normal = Vec3(1,0,0);
    else if (fabs(hit.y - cube.min.y) < eps) normal = Vec3(0,-1,0);
    else if (fabs(hit.y - cube.max.y) < eps) normal = Vec3(0,1,0);
    else if (fabs(hit.z - cube.min.z) < eps) normal = Vec3(0,0,-1);
    else if (fabs(hit.z - cube.max.z) < eps) normal = Vec3(0,0,1);
    else normal = Vec3(0,0,0);

    return true;
}

// Ray-triangle intersection (Möller–Trumbore)
bool RayTracer::intersectTriangle(const Ray& ray, const Triangle& tri, float& t, Vec3& hit, Vec3& normal) {
    const float EPSILON = 1e-6f;
    Vec3 edge1 = tri.v1 - tri.v0;
    Vec3 edge2 = tri.v2 - tri.v0;
    Vec3 h = ray.direction.cross(edge2);
    float a = edge1.dot(h);
    if (fabs(a) < EPSILON) return false;
    float f = 1.0f / a;
    Vec3 s = ray.origin - tri.v0;
    float u = f * s.dot(h);
    if (u < 0.0f || u > 1.0f) return false;
    Vec3 q = s.cross(edge1);
    float v = f * ray.direction.dot(q);
    if (v < 0.0f || u + v > 1.0f) return false;
    t = f * edge2.dot(q);
    if (t > EPSILON) {
        hit = ray.origin + ray.direction * t;
        normal = edge1.cross(edge2).normalized();
        return true;
    }
    return false;
}

void print_progress_bar(float progress) {
    const int barWidth = 50;
    std::cout << "\r[";
    int pos = static_cast<int>(barWidth * progress);
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << int(progress * 100.0) << " %" << std::flush;
}