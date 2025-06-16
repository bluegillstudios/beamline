#include <iostream>
#include <chrono>
#include <string>
#include <iomanip>
#include <ctime>
#include <filesystem>
#include <sstream>
#include <cstdlib>

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
    std::cout << "  beamline <scene.beam> [width height] [--out <file.ppm/png>] [--frame start end step] [--out-stitch <out.mp4>] [--info] [--camera-pos x,y,z] [--camera-look x,y,z] [--no-cleanup] [--stitch-only <pattern>]\n";
    std::cout << "\nExample:\n";
    std::cout << "  beamline scenes/test.beam 800 600\n";
    std::cout << "  beamline scenes/test.beam --out renders/image.png\n";
    std::cout << "  beamline scenes/test.beam --frame 0 1 0.1 --out frames/frame_%04d.png --out-stitch anim.mp4\n\n";
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
    if (scene.spheres.empty() && scene.planes.empty() && scene.triangles.empty() && scene.cubes.empty()) {
        std::cerr << "[WARNING] Scene contains no geometry.\n";
    }
}

void print_scene_summary(const Scene& scene, int width, int height) {
    std::cout << "Resolution:   " << width << "x" << height << "\n";
    std::cout << "Objects:      " << scene.spheres.size() << " spheres, "
              << scene.planes.size() << " planes, "
              << scene.triangles.size() << " triangles, "
              << scene.cubes.size() << " cubes\n";
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

    std::string scene_file;
    std::string output_filename;
    std::string out_stitch_filename;
    std::string stitch_only_pattern;
    bool info_only = false;
    bool camera_pos_override = false;
    bool camera_look_override = false;
    Vec3 camera_pos_override_val;
    Vec3 camera_look_override_val;
    bool do_animation = false;
    bool no_cleanup = false;
    float frame_start = 0.0f, frame_end = 1.0f, frame_step = 0.1f;
    int width = 800, height = 600;

    scene_file = argv[1];

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--info") info_only = true;
        else if (arg == "--out" && i + 1 < argc) output_filename = argv[++i];
        else if (arg == "--frame" && i + 3 < argc) {
            do_animation = true;
            frame_start = std::stof(argv[++i]);
            frame_end   = std::stof(argv[++i]);
            frame_step  = std::stof(argv[++i]);
        } else if (arg == "--out-stitch" && i + 1 < argc) out_stitch_filename = argv[++i];
        else if (arg == "--stitch-only" && i + 1 < argc) stitch_only_pattern = argv[++i];
        else if (arg == "--camera-pos" && i + 1 < argc) {
            std::istringstream ss(argv[++i]);
            float x, y, z;
            char c1, c2;
            if (ss >> x >> c1 >> y >> c2 >> z && c1 == ',' && c2 == ',') {
                camera_pos_override_val = Vec3{x, y, z};
                camera_pos_override = true;
            } else {
                std::cerr << "[ERROR] Invalid --camera-pos format. Use x,y,z\n";
                return 1;
            }
        } else if (arg == "--camera-look" && i + 1 < argc) {
            std::istringstream ss(argv[++i]);
            float x, y, z;
            char c1, c2;
            if (ss >> x >> c1 >> y >> c2 >> z && c1 == ',' && c2 == ',') {
                camera_look_override_val = Vec3{x, y, z};
                camera_look_override = true;
            } else {
                std::cerr << "[ERROR] Invalid --camera-look format. Use x,y,z\n";
                return 1;
            }
        } else if (arg == "--no-cleanup") {
            no_cleanup = true;
        } else if (isdigit(arg[0])) {
            if (i + 1 < argc && isdigit(argv[i + 1][0])) {
                width = std::stoi(arg);
                height = std::stoi(argv[++i]);
            }
        }
    }

    if (!stitch_only_pattern.empty()) {
        std::cout << "Stitching existing frames into: " << stitch_only_pattern << "\n";
        std::stringstream cmd;
        cmd << "ffmpeg -y -framerate 24 -i " << stitch_only_pattern << " -pix_fmt yuv420p " << out_stitch_filename;
        std::system(cmd.str().c_str());
        std::cout << "Done.\n";
        return 0;
    }

    if (!std::filesystem::exists(scene_file)) {
        std::cerr << "[ERROR] File not found: " << scene_file << "\n";
        return 1;
    }

    Scene scene = load_scene_from_file(scene_file);

    if (camera_pos_override) scene.camera.position = camera_pos_override_val;
    if (camera_look_override) scene.camera.lookat = camera_look_override_val;
    if (scene.camera.position == scene.camera.lookat) {
        std::cerr << "[WARNING] Camera position and lookat are identical. Adjusting lookat.\n";
        scene.camera.lookat = scene.camera.position + Vec3{0, 0, -1};
    }

    validate_scene(scene);
    print_scene_summary(scene, width, height);
    if (info_only) {
        std::cout << "\n[INFO MODE] No rendering performed.\n";
        return 0;
    }

    RayTracer tracer(width, height, 4);

    if (do_animation) {
        int frame_idx = 0;
        for (float t = frame_start; t <= frame_end + 1e-4f; t += frame_step) {
            scene.camera.apply_frame(CameraFrame{t});
            tracer.render(scene);

            char buf[256];
            std::snprintf(buf, sizeof(buf), output_filename.c_str(), frame_idx++);
            std::cout << "Saving frame: " << buf << "\n";
            save_image(buf, tracer.getFramebuffer(), width, height);
        }
        if (!out_stitch_filename.empty()) {
            std::stringstream cmd;
            cmd << "ffmpeg -y -framerate 24 -i " << output_filename << " -pix_fmt yuv420p " << out_stitch_filename;
            std::cout << "Stitching frames into video: " << out_stitch_filename << "\n";
            std::system(cmd.str().c_str());

            if (!no_cleanup) {
                std::cout << "Cleaning up PNG frames...\n";
                namespace fs = std::filesystem;
                std::string prefix = output_filename.substr(0, output_filename.find('%'));
                for (const auto& entry : fs::directory_iterator(fs::path(prefix).parent_path())) {
                    if (entry.path().string().find(prefix) != std::string::npos && entry.path().extension() == ".png") {
                        fs::remove(entry.path());
                    }
                }
            } else {
                std::cout << "[INFO] --no-cleanup set. PNG frames preserved.\n";
            }
        }
    } else {
        tracer.render(scene);
        std::string out_file = output_filename.empty() ? get_timestamped_filename("output") : output_filename;
        save_image(out_file, tracer.getFramebuffer(), width, height);
        std::cout << "Saved image to: " << out_file << "\n";
    }

    std::cout << "\nBeamline complete.\n";
    return 0;
}