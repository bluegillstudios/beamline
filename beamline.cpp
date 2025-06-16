#include <iostream>
#include <chrono>
#include <string>
#include <iomanip>
#include <ctime>
#include <filesystem>
#include <sstream>
#include <vector>
#include <cstdio>

#include "loader/SceneLoader.h"
#include "cpu/RayTracer.h"
#include "image/ImageSaver.h"

const std::string BEAMLINE_VERSION = "1.0.1940";

struct CameraKeyframe {
    float time;
    Vec3 position;
    Vec3 lookat;
};

Vec3 lerp(const Vec3& a, const Vec3& b, float t) {
    return a * (1 - t) + b * t;
}

CameraKeyframe interpolateCamera(const std::vector<CameraKeyframe>& keys, float t) {
    if (keys.empty()) throw std::runtime_error("No keyframes defined.");

    if (t <= keys.front().time) return keys.front();
    if (t >= keys.back().time) return keys.back();

    for (size_t i = 0; i < keys.size() - 1; ++i) {
        if (t >= keys[i].time && t <= keys[i + 1].time) {
            float local_t = (t - keys[i].time) / (keys[i + 1].time - keys[i].time);
            return CameraKeyframe{
                t,
                lerp(keys[i].position, keys[i + 1].position, local_t),
                lerp(keys[i].lookat, keys[i + 1].lookat, local_t)
            };
        }
    }
    return keys.back();
}

void print_banner() {
    std::cout << "\n";
    std::cout << "=======================================\n";
    std::cout << "   Beamline\n";
    std::cout << "   Version " << BEAMLINE_VERSION << "\n";
    std::cout << "   An open source command-line renderer\n";
    std::cout << "=======================================\n\n";
}

void print_usage() {
    std::cout << "Usage:\n";
    std::cout << "  beamline <scene.beam> [width height] [--out <file.png>] [--info] [--animate <duration_secs> <fps>]\n";
    std::cout << "\nExample:\n";
    std::cout << "  beamline scenes/test.beam 800 600 --out output.png\n";
    std::cout << "  beamline scenes/test.beam --info\n";
    std::cout << "  beamline scenes/test.beam 800 600 --out frame_%04d.png --animate 2 30\n\n";
}

std::string get_timestamped_filename(const std::string& base) {
    std::time_t now = std::time(nullptr);
    std::tm* tm_info = std::localtime(&now);
    char buffer[32];
    std::strftime(buffer, 32, "%Y-%m-%d_%H%M", tm_info);
    return base + "_" + buffer + ".png";
}

void validate_scene(const Scene& scene) {
    if (scene.camera.position == scene.camera.lookat) {
        std::cerr << "[WARNING] Camera position and lookat are identical.\n";
    }
    if (scene.lights.empty()) {
        std::cerr << "[WARNING] No lights in scene. It will render black.\n";
    }
    if (scene.spheres.empty() && scene.planes.empty()) {
        std::cerr << "[WARNING] Scene contains no geometry.\n";
    }
}

void print_scene_summary(const Scene& scene, int width, int height) {
    std::cout << "Resolution:   " << width << "x" << height << "\n";
    std::cout << "Objects:      " << scene.spheres.size() << " spheres, "
              << scene.planes.size() << " planes\n";
    std::cout << "Lights:       " << scene.lights.size() << "\n";
    std::cout << "Camera Pos:   (" << scene.camera.position.x << ", "
              << scene.camera.position.y << ", " << scene.camera.position.z << ")\n";
    std::cout << "Camera Look:  (" << scene.camera.lookat.x << ", "
              << scene.camera.lookat.y << ", " << scene.camera.lookat.z << ")\n";
}

int main(int argc, char* argv[]) {
    print_banner();

    if (argc < 2) {
        print_usage();
        return 1;
    }

    std::string scene_file = argv[1];
    bool info_only = false;
    int width = 800, height = 600;
    std::string output_filename;
    bool animate = false;
    float anim_duration = 0.f;
    int anim_fps = 30;

    // Camera overrides
    bool camera_pos_override = false;
    bool camera_look_override = false;
    Vec3 camera_pos_override_val;
    Vec3 camera_look_override_val;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--info") {
            info_only = true;
        } else if (arg == "--out" && i + 1 < argc) {
            output_filename = argv[++i];
        } else if (isdigit(arg[0])) {
            if (i + 1 < argc && isdigit(argv[i + 1][0])) {
                width = std::stoi(arg);
                height = std::stoi(argv[++i]);
            }
        } else if (arg == "--animate" && i + 2 < argc) {
            animate = true;
            anim_duration = std::stof(argv[++i]);
            anim_fps = std::stoi(argv[++i]);
        } else if (arg == "--camera-pos" && i + 1 < argc) {
            std::string val = argv[++i];
            std::replace(val.begin(), val.end(), ',', ' ');
            std::istringstream ss(val);
            float x, y, z;
            if (ss >> x >> y >> z) {
                camera_pos_override_val = Vec3{x, y, z};
                camera_pos_override = true;
            } else {
                std::cerr << "[ERROR] Invalid format for --camera-pos. Use x,y,z\n";
                return 1;
            }
        } else if (arg == "--camera-look" && i + 1 < argc) {
            std::string val = argv[++i];
            std::replace(val.begin(), val.end(), ',', ' ');
            std::istringstream ss(val);
            float x, y, z;
            if (ss >> x >> y >> z) {
                camera_look_override_val = Vec3{x, y, z};
                camera_look_override = true;
            } else {
                std::cerr << "[ERROR] Invalid format for --camera-look. Use x,y,z\n";
                return 1;
            }
        }
    }

    if (!std::filesystem::exists(scene_file)) {
        std::cerr << "Error: File not found: " << scene_file << "\n";
        return 1;
    }

    auto load_start = std::chrono::high_resolution_clock::now();
    Scene scene = load_scene_from_file(scene_file);
    auto load_end = std::chrono::high_resolution_clock::now();

    double load_time = std::chrono::duration<double>(load_end - load_start).count();

    // Override camera if flags set
    if (camera_pos_override) scene.camera.position = camera_pos_override_val;
    if (camera_look_override) scene.camera.lookat = camera_look_override_val;

    if (scene.camera.position == scene.camera.lookat) {
        std::cerr << "[WARNING] Camera position and lookat are identical. Adjusting lookat.\n";
        scene.camera.lookat = scene.camera.position + Vec3{0, 0, -1};
    }

    std::cout << "Scene loaded: " << scene_file << " (" << load_time << " sec)\n";

    validate_scene(scene);
    print_scene_summary(scene, width, height);

    if (info_only) {
        std::cout << "\n[INFO MODE] No rendering performed.\n";
        return 0;
    }

    if (!animate) {
        // Single frame render
        std::cout << "\nRendering...\n";
        auto render_start = std::chrono::high_resolution_clock::now();

        RayTracer tracer(width, height, 4);
        tracer.render(scene);

        auto render_end = std::chrono::high_resolution_clock::now();
        double render_time = std::chrono::duration<double>(render_end - render_start).count();

        std::string output_file = output_filename.empty()
            ? get_timestamped_filename("output")
            : output_filename;

        std::cout << "Saving to: " << output_file << "\n";

        auto save_start = std::chrono::high_resolution_clock::now();
        save_image(output_file, tracer.getFramebuffer(), width, height);
        auto save_end = std::chrono::high_resolution_clock::now();

        double save_time = std::chrono::duration<double>(save_end - save_start).count();

        std::cout << std::fixed << std::setprecision(3);
        std::cout << "\n--- Timing Summary ---\n";
        std::cout << "Scene load:   " << load_time << " sec\n";
        std::cout << "Render time:  " << render_time << " sec\n";
        std::cout << "Save image:   " << save_time << " sec\n";
        std::cout << "Total:        " << (load_time + render_time + save_time) << " sec\n";

    } else {
        // Animate!
        std::cout << "\nAnimation mode: duration = " << anim_duration << "s, FPS = " << anim_fps << "\n";

        int total_frames = static_cast<int>(anim_duration * anim_fps);
        float frame_time_step = anim_duration / total_frames;

        // Define camera keyframes here (example)
        std::vector<CameraKeyframe> camera_keys = {
            {0.0f, scene.camera.position, scene.camera.lookat},
            {anim_duration / 2.f, scene.camera.position + Vec3{5.f, 2.f, 0.f}, scene.camera.lookat},
            {anim_duration, scene.camera.position + Vec3{10.f, 0.f, 0.f}, scene.camera.lookat},
        };

        for (int frame = 0; frame < total_frames; ++frame) {
            float t = frame * frame_time_step;

            CameraKeyframe cam = interpolateCamera(camera_keys, t);
            scene.camera.position = cam.position;
            scene.camera.lookat = cam.lookat;

            std::cout << "Rendering frame " << frame << " / " << total_frames << " (t=" << t << ")\n";

            RayTracer tracer(width, height, 4);
            tracer.render(scene);

            std::string frame_file;
            if (!output_filename.empty() && output_filename.find("%d") != std::string::npos) {
                char buf[512];
                std::snprintf(buf, sizeof(buf), output_filename.c_str(), frame);
                frame_file = buf;
            } else if (!output_filename.empty()) {
                // If output filename specified but no %d, append frame index
                frame_file = output_filename + "_" + std::to_string(frame) + ".png";
            } else {
                frame_file = "frame_" + std::to_string(frame) + ".png";
            }

            save_image(frame_file, tracer.getFramebuffer(), width, height);
        }
    }

    std::cout << "\nBeamline complete.\n";
    return 0;
}
