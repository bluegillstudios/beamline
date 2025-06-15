#include "SceneLoader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <algorithm>

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

Scene load_scene_from_file(const std::string& filename) {
    Scene scene;

    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error: cannot open scene file '" << filename << "'\n";
        return scene;
    }

    std::string line;
    std::string current_section;
    std::unordered_map<std::string, std::string> properties;

    auto add_object = [&](const std::unordered_map<std::string, std::string>& props) {
        if (props.empty()) return;

        auto it_type = props.find("type");

        if (current_section == "Camera") {
            if (props.find("position") != props.end())
                scene.camera.position = parseVec3(props.at("position"));
            if (props.find("lookat") != props.end())
                scene.camera.lookat = parseVec3(props.at("lookat"));
            return;
        }

        if (it_type == props.end()) return;

        const std::string& type = it_type->second;

        try {
            if (type == "sphere") {
                Sphere s;
                s.center = parseVec3(props.at("center"));
                s.radius = parseFloat(props.at("radius"));
                s.material.diffuse_color = parseVec3(props.at("diffuse"));
                s.material.reflectivity = parseFloat(props.at("reflectivity"));
                scene.spheres.push_back(s);
            }
            else if (type == "plane") {
                Plane p;
                p.point = parseVec3(props.at("point"));
                p.normal = parseVec3(props.at("normal"));
                p.material.diffuse_color = parseVec3(props.at("diffuse"));
                p.material.reflectivity = parseFloat(props.at("reflectivity"));
                scene.planes.push_back(p);
            }
            else if (type == "point") {
                Light l;
                l.position = parseVec3(props.at("position"));
                l.color = parseVec3(props.at("color"));
                scene.lights.push_back(l);
            }
        } catch (const std::out_of_range& e) {
            std::cerr << "Error parsing section [" << current_section << "]: missing required properties.\n";
        }
    };

    while (std::getline(file, line)) {
        line = trim(line);

        if (line.empty() || line[0] == '#')
            continue;

        if (line[0] == '[') {
            // New section
            add_object(properties);
            properties.clear();

            size_t end = line.find(']');
            if (end == std::string::npos) {
                std::cerr << "Warning: malformed section header: " << line << "\n";
                current_section.clear();
            } else {
                current_section = line.substr(1, end - 1);
            }
        } else {
            // key=value line
            size_t eq = line.find('=');
            if (eq == std::string::npos) {
                std::cerr << "Warning: malformed key=value line: " << line << "\n";
                continue;
            }

            std::string key = trim(line.substr(0, eq));
            std::string val = trim(line.substr(eq + 1));

            properties[key] = val;
        }
    }
    add_object(properties); // Add last

    return scene;
}