#include <iostream>
#include <chrono>
#include <string>
#include <iomanip>
#include <ctime>
#include <filesystem>
#include "../loader/SceneLoader.h"
#include "../cpu/RayTracer.h"
#include "../image/ImageSaver.h"

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
    std::cout << "  beamline <scene.beam> [width height]\n";
    std::cout << "Options:\n";
    std::cout << "  --info         Only print scene summary\n";
    std::cout << "\nExample:\n";
    std::cout << "  beamline scenes/test.beam 800 600\n";
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

int main(int argc, char* argv[]) {
    print_banner();

    if (argc < 2) {
        print_usage();
        return 1;
    }

    std::string scene_file = argv[1];
    bool info_only = false;

    int width = 800, height = 600;

    if (argc == 3 && std::string(argv[2]) == "--info") {
        info_only = true;
    } else if (argc >= 4) {
        width = std::stoi(argv[2]);
        height = std::stoi(argv[3]);
    }

    if (!std::filesystem::exists(scene_file)) {
        std::cerr << "Error: File not found: " << scene_file << "\n";
        return 1;
    }

    auto load_start = std::chrono::high_resolution_clock::now();
    Scene scene = load_scene_from_file(scene_file);
    auto load_end = std::chrono::high_resolution_clock::now();

    double load_time = std::chrono::duration<double>(load_end - load_start).count();

    std::cout << "Scene loaded: " << scene_file << " (" << load_time << " sec)\n";

    validate_scene(scene);
    print_scene_summary(scene, width, height);

    if (info_only) {
        std::cout << "\n[INFO MODE] No rendering performed.\n";
        return 0;
    }

    std::cout << "\nRendering...\n";
    auto render_start = std::chrono::high_resolution_clock::now();

    RayTracer tracer(width, height, 4);
    tracer.render(scene);

    auto render_end = std::chrono::high_resolution_clock::now();
    double render_time = std::chrono::duration<double>(render_end - render_start).count();

    std::string output_file = get_timestamped_filename("output");
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

    std::cout << "\nBeamline complete.\n";
    return 0;
}
