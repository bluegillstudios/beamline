#pragma once
#include <string>
#include <vector>
#include "../Vec3.h"       

struct Material {
    Vec3 diffuse_color;
    float reflectivity;
};

struct Sphere {
    Vec3 center;
    float radius;
    Material material;
};

struct Plane {
    Vec3 point;
    Vec3 normal;
    Material material;
};

struct Light {
    Vec3 position;
    Vec3 color;
};

struct Camera {
    Vec3 position;
    Vec3 lookat;
};

struct Scene {
    std::vector<Sphere> spheres;
    std::vector<Plane> planes;
    std::vector<Light> lights;
    Camera camera;
};

Scene load_scene_from_file(const std::string& filename);