#pragma once
#include <string>
#include <vector>
#include "../Vec3.h"

void save_image(const std::string& filename, const std::vector<Vec3>& framebuffer, int width, int height);
