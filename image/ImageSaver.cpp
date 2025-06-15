#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "ImageSaver.h"
#include <fstream>
#include <iostream>
#include <cctype>
#include <algorithm>

static void save_ppm(const std::string& filename, const std::vector<Vec3>& framebuffer, int width, int height) {
    std::ofstream ofs(filename);
    if (!ofs) {
        std::cerr << "[ERROR] Couldn't open file: " << filename << "\n";
        return;
    }

    ofs << "P3\n" << width << " " << height << "\n255\n";
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const Vec3& color = framebuffer[y * width + x];
            int r = std::min(255, static_cast<int>(255.999f * std::clamp(color.x, 0.0f, 1.0f)));
            int g = std::min(255, static_cast<int>(255.999f * std::clamp(color.y, 0.0f, 1.0f)));
            int b = std::min(255, static_cast<int>(255.999f * std::clamp(color.z, 0.0f, 1.0f)));
            ofs << r << ' ' << g << ' ' << b << '\n';
        }
    }

    std::cout << "[OK] Saved PPM image: " << filename << "\n";
}

static void save_png(const std::string& filename, const std::vector<Vec3>& framebuffer, int width, int height) {
    std::vector<unsigned char> image(3 * width * height);
    for (int i = 0; i < width * height; ++i) {
        image[3 * i + 0] = static_cast<unsigned char>(255.999f * std::clamp(framebuffer[i].x, 0.0f, 1.0f));
        image[3 * i + 1] = static_cast<unsigned char>(255.999f * std::clamp(framebuffer[i].y, 0.0f, 1.0f));
        image[3 * i + 2] = static_cast<unsigned char>(255.999f * std::clamp(framebuffer[i].z, 0.0f, 1.0f));
    }

    if (stbi_write_png(filename.c_str(), width, height, 3, image.data(), width * 3)) {
        std::cout << "[OK] Saved PNG image: " << filename << "\n";
    } else {
        std::cerr << "[ERROR] Failed to save PNG image.\n";
    }
}

void save_image(const std::string& filename, const std::vector<Vec3>& framebuffer, int width, int height) {
    std::string ext = filename.substr(filename.find_last_of('.') + 1);
    for (auto& c : ext) c = std::tolower(c);

    if (ext == "ppm") {
        save_ppm(filename, framebuffer, width, height);
    } else if (ext == "png") {
        save_png(filename, framebuffer, width, height);
    } else {
        std::cerr << "[ERROR] Unsupported format: ." << ext << "\n";
    }
}