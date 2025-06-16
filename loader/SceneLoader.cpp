#include "SceneLoader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <map>

// Helpers

static Vec3 parseVec3(const std::string& s) {
    std::istringstream ss(s);
    float x = 0, y = 0, z = 0;
    ss >> x >> y >> z;
    return Vec3{x, y, z};
}

static float parseFloat(const std::string& s) {
    return std::stof(s);
}

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    return s.substr(start, end - start + 1);
}

inline Vec3 parse_vec3(const std::string& line) {
    std::istringstream ss(line);
    float x, y, z;
    ss >> x >> y >> z;
    return Vec3(x, y, z);
}

Scene load_scene_from_file(const std::string& filename) {
    Scene scene;
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Could not open scene file: " << filename << std::endl;
        return scene;
    }

    std::string line, section;
    std::map<std::string, std::string> current;

    auto process_section = [&](const std::string& section_name, const std::map<std::string, std::string>& data) {
        if (section_name == "AmbientLight") {
            if (data.count("color")) {
                scene.ambient_light = parse_vec3(data.at("color"));
            }
        } else if (data.count("type")) {
            const std::string& type = data.at("type");
            if (type == "sphere") {
                Sphere s;
                s.center = parse_vec3(data.at("center"));
                s.radius = std::stof(data.at("radius"));
                s.material.diffuse_color = parse_vec3(data.at("diffuse"));
                s.material.reflectivity = std::stof(data.at("reflectivity"));
                if (data.count("emission"))
                    s.material.emission = parse_vec3(data.at("emission"));
                if (data.count("ior"))
                    s.material.ior = std::stof(data.at("ior"));
                scene.spheres.push_back(s);
            } else if (type == "plane") {
                Plane p;
                p.point = parse_vec3(data.at("point"));
                p.normal = parse_vec3(data.at("normal"));
                p.material.diffuse_color = parse_vec3(data.at("diffuse"));
                p.material.reflectivity = std::stof(data.at("reflectivity"));
                if (data.count("emission"))
                    p.material.emission = parse_vec3(data.at("emission"));
                if (data.count("ior"))
                    p.material.ior = std::stof(data.at("ior"));
                scene.planes.push_back(p);
            } else if (type == "point") {
                Light l;
                l.position = parse_vec3(data.at("position"));
                l.color = parse_vec3(data.at("color"));
                scene.lights.push_back(l);
            } else if (type == "triangle") {
                Triangle t;
                t.v0 = parse_vec3(data.at("v0"));
                t.v1 = parse_vec3(data.at("v1"));
                t.v2 = parse_vec3(data.at("v2"));
                t.material.diffuse_color = parse_vec3(data.at("diffuse"));
                t.material.reflectivity = std::stof(data.at("reflectivity"));
                if (data.count("emission"))
                    t.material.emission = parse_vec3(data.at("emission"));
                if (data.count("ior"))
                    t.material.ior = std::stof(data.at("ior"));
                scene.triangles.push_back(t);
            } else if (type == "cube") {
                Cube c;
                c.min = parse_vec3(data.at("min"));
                c.max = parse_vec3(data.at("max"));
                c.material.diffuse_color = parse_vec3(data.at("diffuse"));
                c.material.reflectivity = std::stof(data.at("reflectivity"));
                if (data.count("emission"))
                    c.material.emission = parse_vec3(data.at("emission"));
                if (data.count("ior"))
                    c.material.ior = std::stof(data.at("ior"));
                scene.cubes.push_back(c);
            }
        }
    };

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        if (line[0] == '[' && line.back() == ']') {
            if (!section.empty() && !current.empty()) {
                process_section(section, current);
                current.clear();
            }
            section = line.substr(1, line.size() - 2);
        } else {
            size_t eq = line.find('=');
            if (eq != std::string::npos) {
                std::string key = line.substr(0, eq);
                std::string value = line.substr(eq + 1);
                key.erase(0, key.find_first_not_of(" "));
                key.erase(key.find_last_not_of(" ") + 1);
                value.erase(0, value.find_first_not_of(" "));
                value.erase(value.find_last_not_of(" ") + 1);
                current[key] = value;
            }
        }
    }

    // Process last section after loop ends
    if (!section.empty() && !current.empty()) {
        process_section(section, current);
    }

    return scene;
}