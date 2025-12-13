#include <mpi.h>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <cstring>
#include <cstdlib>

void median_filter_sequential(const std::vector<uint8_t> &image,
                              std::vector<uint8_t> &output,
                              int width, int height,
                              int k)
{
    int r = k / 2;
    output.resize(width * height);

    std::vector<uint8_t> window;
    window.reserve(k * k);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            window.clear();

            for (int dy = -r; dy <= r; ++dy) {
                int ny = y + dy;
                if (ny < 0 || ny >= height) continue;

                for (int dx = -r; dx <= r; ++dx) {
                    int nx = x + dx;
                    if (nx < 0 || nx >= width) continue;

                    window.push_back(image[ny * width + nx]);
                }
            }

            std::sort(window.begin(), window.end());
            uint8_t median_value = window[window.size() / 2];
            output[y * width + x] = median_value;
        }
    }
}



void median_filter_parallel(const std::vector<uint8_t> &image,std::vector<uint8_t>& output,
                           int width, int height, int k){

      MPI_Init(NULL, NULL);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    double t0 = MPI_Wtime();

    int r = k / 2;
    int rows_processes = height/size;

    std::vector<uint8_t> full_image, full_output;
    std::vector<uint8_t> local_image, local_output, extended_image;

    if (rank == 0) {
        // Master: Generate random image
        full_image.resize(width * height);
        for (auto& p : full_image) {
            p = static_cast<uint8_t>(rand() % 256);
        }
        full_output.resize(width * height);

        std::cout << "Master: Generated " << width << "x" << height
                  << " image, distributing to " << size << " processes" << std::endl;
    }

    MPI_Bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&k, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Allocate local buffers
    local_image.resize(rows_processes * width);
    local_output.resize(rows_processes * width);
    extended_image.resize((rows_processes + 2 * r) * width);

    MPI_Scatter(rank == 0 ? full_image.data() : NULL,
                rows_processes , MPI_UNSIGNED_CHAR,local_image.data(),rows_processes, MPI_UNSIGNED_CHAR,
                0, MPI_COMM_WORLD);

    MPI_Request reqs[4];
    int req_count = 0;

    // ----- TOP HALO -----
    if (rank > 0) {
        // Post nonblocking receive from above
        MPI_Irecv(extended_image.data(), r * width, MPI_UNSIGNED_CHAR,
                  rank - 1, 0, MPI_COMM_WORLD, &reqs[req_count++]);

        // Post nonblocking send of my top rows to above
        MPI_Isend(local_image.data(), r * width, MPI_UNSIGNED_CHAR,
                  rank - 1, 1, MPI_COMM_WORLD, &reqs[req_count++]);
    } else {
        // First process: zero-pad top
        std::fill(extended_image.begin(), extended_image.begin() + r * width, 0);
    }

    // ----- BOTTOM HALO -----
    if (rank < size - 1) {
        // Post nonblocking send of my bottom rows to below
        MPI_Isend(local_image.data() + (rows_processes - r) * width,
                  r * width, MPI_UNSIGNED_CHAR,
                  rank + 1, 0, MPI_COMM_WORLD, &reqs[req_count++]);

        // Post nonblocking receive from below
        MPI_Irecv(extended_image.data() + (r + rows_processes) * width,
                  r * width, MPI_UNSIGNED_CHAR,
                  rank + 1, 1, MPI_COMM_WORLD, &reqs[req_count++]);
    } else {
        // Last process: zero-pad bottom
        std::fill(extended_image.end() - r * width, extended_image.end(), 0);
    }

    // ----- WAIT FOR ALL NONBLOCKING OPS -----
    if (req_count > 0) {
        MPI_Waitall(req_count, reqs, MPI_STATUSES_IGNORE);
    }

    std::vector<uint8_t> final_output(rows_processes * width);
    for (int i = 0; i < rows_processes; ++i) {
        std::copy(local_output.begin() + (r + i) * width,
                  local_output.begin() + (r + i + 1) * width,
                  final_output.begin() + i * width);
    }

    // Gather filtered results
    MPI_Gather(final_output.data(), rows_processes * width, MPI_UNSIGNED_CHAR,
               rank == 0 ? full_output.data() : NULL,
               rows_processes * width, MPI_UNSIGNED_CHAR,
               0, MPI_COMM_WORLD);

    double t1 = MPI_Wtime();
    if (rank == 0) {
        std::cout << "Master: Received filtered image from all processes" << std::endl;
        // Verify: first few pixels should be filtered values
        std::cout << "Sample output: "
                  << (int)full_output[0] << ", "
                  << (int)full_output[1] << ", "
                  << (int)full_output[width] << std::endl;
        std::cout << "Parallel time: " << (t1 - t0) << " s\n";
    }

    MPI_Finalize();


  }

int main(int argc, char *argv[]) {
    MPI_Init(NULL, NULL);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int width  = 4096;
    int height = 4096;
    int k      = 5;

    std::vector<uint8_t> image;

    if (rank == 0) {
        image.resize(width * height);
        std::srand(12345);
        for (auto &p : image) {
            p = static_cast<uint8_t>(std::rand() % 256);
        }
    }

    // Broadcast image to all ranks so parallel can use the same data
    if (rank != 0) {
        image.resize(width * height);
    }
    MPI_Bcast(image.data(), width * height, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    // Measure sequential time on rank 0 only
    double seq_time = 0.0;
    if (rank == 0) {
        std::vector<uint8_t> seq_out;
        double t0 = MPI_Wtime();
        median_filter_sequential(image, seq_out, width, height, k);
        double t1 = MPI_Wtime();
        seq_time = t1 - t0;
        std::cout << "Sequential time: " << seq_time << " s\n";
    }

    // Ensure all ranks start parallel at same time
    MPI_Barrier(MPI_COMM_WORLD);

    // Measure parallel time (all ranks participate, rank 0 reports)
    double t0p = MPI_Wtime();
    std::vector<uint8_t> par_out;
    median_filter_parallel(image, par_out, width, height, k);
    double t1p = MPI_Wtime();
    double par_time = t1p - t0p;

    if (rank == 0) {
        std::cout << "Parallel time:   " << par_time << " s\n";
        if (par_time > 0.0) {
            std::cout << "Speedup (seq/par): " << (seq_time / par_time) << "x\n";
        }
    }

    MPI_Finalize();
    return 0;
}