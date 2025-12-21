#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <chrono>
#include <fstream>
using namespace std;
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
    int width  = 4096;
    int height = 4096;
    int k      = 5;


    std::vector<uint8_t> image(width * height);
    std::vector<uint8_t> output;


    std::cout << "Initializing image..." << std::endl;
    for (auto& p : image) {
        p = static_cast<uint8_t>(rand() % 256);
    }


    system("mkdir check > NUL 2>&1");


    // Save input image for parallel version
    std::ofstream in_file("check/input_image.raw", std::ios::binary);
    in_file.write(reinterpret_cast<const char*>(image.data()), image.size());
    in_file.close();


    std::cout << "Running sequential median filter..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();


    median_filter_sequential(image, output, width, height, k);


    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> time = end - start;


    double seq_time = time.count();
    std::cout << "Done.\nTime: " << seq_time << " s" << std::endl;


    // === write sequential output here ===
    std::ofstream out("check/seq_output.raw", std::ios::binary);
    out.write(reinterpret_cast<const char*>(output.data()),
              output.size());
    out.close();
}


