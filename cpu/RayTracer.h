#pragma once
#include <vector>
#include "../Vec3.h"
#include "../loader/SceneLoader.h"

struct Ray {
    Vec3 origin;
    Vec3 direction;

    Ray(const Vec3& o, const Vec3& d) : origin(o), direction(d.normalized()) {}
};

class RayTracer {
public:
    RayTracer(int width, int height, int maxDepth = 4);

    void render(const Scene& scene);
    const std::vector<Vec3>& getFramebuffer() const;

private:
    int width, height;
    int maxDepth;
    std::vector<Vec3> framebuffer;

    Vec3 trace(const Ray& ray, const Scene& scene, int depth);
    bool intersect(const Ray& ray, const Scene& scene, Vec3& hitPoint, Vec3& normal, Material& mat);
    bool intersectSphere(const Ray& ray, const Sphere& sphere, float& t, Vec3& hit, Vec3& normal);
    bool intersectPlane(const Ray& ray, const Plane& plane, float& t, Vec3& hit, Vec3& normal);
    bool intersectCube(const Ray& ray, const Cube& cube, float& t, Vec3& hit, Vec3& normal);
    bool intersectTriangle(const Ray& ray, const Triangle& tri, float& t, Vec3& hit, Vec3& normal);
};