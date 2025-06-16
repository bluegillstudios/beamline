#include <iostream>
#include <chrono>
#include <string>
#include <iomanip>
#include <ctime>
#include <filesystem>
#include <sstream>
#include <algorithm>
#include <cstdlib>  // for std::system()
#include "loader/SceneLoader.h"
#include "cpu/RayTracer.h"
#include "image/ImageSaver.h"

const std::string BEAMLINE_VERSION = "1.0.1940";

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
    std::cout << "  beamline <scene.beam> [width height] [--out <file.ppm/png or pattern>]\n";
    std::cout << "           [--animate <seconds> <fps>] [--out-stitch [output.mp4]] [--info]\n";
    std::cout << "\nExample:\n";
    std::cout << "  beamline scenes/test.beam 800 600\n";
    std::cout << "  beamline scenes/test.beam --out output.png\n";
    std::cout << "  beamline scenes/test.beam --animate 5 30 --out frame_%04d.png --out-stitch output.mp4\n";
    std::cout << "  beamline scenes/test.beam --info\n\n";
}

std::string get_timestamped_filename(const std::string& base) {
    std::time_t now = std::time(nullptr);
    std::tm* tm_info = std::localtime(&now);
    char buffer[32];
    std::strftime(buffer, 32, "%Y-%m-%d_%H%M", tm_info);
    return base + "_" + buffer + ".ppm";
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

// Simple linear interpolation for Vec3
Vec3 lerp(const Vec3& a, const Vec3& b, float t) {
    return a * (1.0f - t) + b * t;
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
    bool out_stitch = false;
    std::string stitch_output = "animation.mp4";
    bool animate = false;
    float anim_seconds = 0.0f;
    int anim_fps = 30;

    // Camera override
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
        } else if (arg == "--camera-pos" && i + 1 < argc) {
            std::string val = argv[++i];
            std::istringstream ss(val);
            float x, y, z;
            char comma1, comma2;
            if (ss >> x >> comma1 >> y >> comma2 >> z && comma1 == ',' && comma2 == ',') {
                camera_pos_override_val = Vec3{x, y, z};
                camera_pos_override = true;
            } else {
                std::cerr << "[ERROR] Invalid format for --camera-pos. Use x,y,z\n";
                return 1;
            }
        } else if (arg == "--camera-look" && i + 1 < argc) {
            std::string val = argv[++i];
            std::istringstream ss(val);
            float x, y, z;
            char comma1, comma2;
            if (ss >> x >> comma1 >> y >> comma2 >> z && comma1 == ',' && comma2 == ',') {
                camera_look_override_val = Vec3{x, y, z};
                camera_look_override = true;
            } else {
                std::cerr << "[ERROR] Invalid format for --camera-look. Use x,y,z\n";
                return 1;
            }
        } else if (arg == "--animate" && i + 2 < argc) {
            animate = true;
            anim_seconds = std::stof(argv[++i]);
            anim_fps = std::stoi(argv[++i]);
            if (anim_seconds <= 0 || anim_fps <= 0) {
                std::cerr << "[ERROR] Invalid animation parameters.\n";
                return 1;
            }
        } else if (arg == "--out-stitch") {
            out_stitch = true;
            // Optional filename
            if (i + 1 < argc && argv[i+1][0] != '-') {
                stitch_output = argv[++i];
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
    if (camera_pos_override) {
        scene.camera.position = camera_pos_override_val;
    }
    if (camera_look_override) {
        scene.camera.lookat = camera_look_override_val;
    }
    // Validate camera position and lookat
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
        std::cout << "Scene load:   " << load_time   << " sec\n";
        std::cout << "Render time:  " << render_time << " sec\n";
        std::cout << "Save image:   " << save_time   << " sec\n";
        std::cout << "Total:        " << (load_time + render_time + save_time) << " sec\n";

    } else {
        // Animation mode
        int total_frames = static_cast<int>(anim_seconds * anim_fps);
        std::cout << "\nAnimation mode: " << anim_seconds << "s @ " << anim_fps << "fps = "
                  << total_frames << " frames\n";

        // We require output filename pattern to contain printf-style %d for frame index
        if (output_filename.empty() || output_filename.find("%") == std::string::npos) {
            std::cerr << "[ERROR] In animation mode, --out filename must contain a printf-style frame number pattern, e.g. frame_%04d.png\n";
            return 1;
        }

        RayTracer tracer(width, height, 4);

        // For demonstration, we animate camera.position linearly from start to end over frames
        Vec3 start_pos = scene.camera.position;
        Vec3 end_pos = scene.camera.position + Vec3{0, 0, -5};  // Move forward 5 units
        Vec3 start_look = scene.camera.lookat;
        Vec3 end_look = scene.camera.lookat;

        auto anim_start = std::chrono::high_resolution_clock::now();

        for (int frame = 0; frame < total_frames; ++frame) {
            float t = float(frame) / float(total_frames - 1);
            scene.camera.position = lerp(start_pos, end_pos, t);
            scene.camera.lookat = lerp(start_look, end_look, t);

            tracer.render(scene);

            char filename_buf[256];
            std::snprintf(filename_buf, sizeof(filename_buf), output_filename.c_str(), frame);

            save_image(filename_buf, tracer.getFramebuffer(), width, height);

            std::cout << "Rendered and saved frame " << frame << " to " << filename_buf << "\n";
        }

        auto anim_end = std::chrono::high_resolution_clock::now();
        double anim_time = std::chrono::duration<double>(anim_end - anim_start).count();
        std::cout << "\nAnimation render time: " << anim_time << " sec\n";

        if (out_stitch) {
            std::cout << "Stitching frames into video: " << stitch_output << "\n";

            // Compose ffmpeg command
            std::string ffmpeg_cmd =
                "ffmpeg -y -framerate " + std::to_string(anim_fps) +
                " -i " + output_filename +
                " -c:v libx264 -pix_fmt yuv420p " + stitch_output;

            int ret = std::system(ffmpeg_cmd.c_str());
            if (ret != 0) {
                std::cerr << "[ERROR] ffmpeg command failed with exit code " << ret << "\n";
            } else {
                std::cout << "Video file created successfully: " << stitch_output << "\n";
            }
        }
    }

    std::cout << "\nBeamline complete.\n";
    return 0;
}