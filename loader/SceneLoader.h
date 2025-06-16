#pragma once
#include <string>
#include <vector>
#include "../Vec3.h"       

struct Material {
    Vec3 diffuse_color;
    float reflectivity;
    float ior;            
    Vec3 emission;         // New emission field 

    Material() : diffuse_color(1,1,1), reflectivity(0), ior(1.0f), emission(0,0,0) {}
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

struct Cube {
    Vec3 min, max;
    Material material;
};

struct Triangle {
    Vec3 v0, v1, v2;
    Material material;
};

struct Scene {
    std::vector<Sphere> spheres;
    std::vector<Plane> planes;
    std::vector<Light> lights;
    std::vector<Cube> cubes;
    std::vector<Triangle> triangles;
    Vec3 ambient_light;    // New ambient light color
    Camera camera;

    Scene() : ambient_light(0.1f, 0.1f, 0.1f) {} // default ambient
};

Scene load_scene_from_file(const std::string& filename);
