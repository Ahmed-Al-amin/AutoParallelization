#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <chrono>

void median_filter_sequential(const std::vector<uint8_t>& image,
                              std::vector<uint8_t>& output,
                              int width, int height, int k)
{
    int r = k / 2;
    output.resize(width * height);

    std::vector<uint8_t> window;
    window.reserve(k * k);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {

            window.clear();

            for (int dy = -r; dy <= r; dy++) {
                int ny = y + dy;
                if (ny < 0 || ny >= height) continue;

                for (int dx = -r; dx <= r; dx++) {
                    int nx = x + dx;
                    if (nx < 0 || nx >= width) continue;

                    window.push_back(image[ny * width + nx]);
                }
            }

            std::sort(window.begin(), window.end());
            output[y * width + x] = window[window.size() / 2];
        }
    }
}

int main() {
    int width  = 4096*2;
    int height = 4096*2;
    int k      = 5;

    std::vector<uint8_t> image(width * height);
    std::vector<uint8_t> output;

    std::cout << "Initializing image..." << std::endl;
    for (auto& p : image) {
        p = static_cast<uint8_t>(rand() % 256);
    }

    std::cout << "Running sequential median filter..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    median_filter_sequential(image, output, width, height, k);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> time = end - start;

    std::cout << "Done.\nTime: " << time.count() << " s" << std::endl;
    return 0;
}
