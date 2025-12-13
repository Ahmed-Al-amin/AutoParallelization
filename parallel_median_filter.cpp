#include <mpi.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <cstdlib>

void median_filter_mpi(const std::vector<uint8_t>& local_image,
                       std::vector<uint8_t>& local_output,
                       int width, int local_height, int k)
{
    int r = k / 2;
    local_output.resize(local_height * width);

    std::vector<uint8_t> window;
    window.reserve(k * k);

    for (int y = r; y < local_height - r; y++) {
        for (int x = 0; x < width; x++) {

            window.clear();

            for (int dy = -r; dy <= r; dy++) {
                for (int dx = -r; dx <= r; dx++) {
                    int nx = x + dx;
                    if (nx < 0 || nx >= width) continue;

                    window.push_back(
                        local_image[(y + dy) * width + nx]
                    );
                }
            }

            std::sort(window.begin(), window.end());
            local_output[y * width + x] = window[window.size() / 2];
        }
    }
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int width  = 4096 * 2;
    int height = 4096 * 2;
    int k      = 5;
    int r      = k / 2;

    int rows_per_rank = height / size;

    std::vector<uint8_t> full_image, full_output;

    if (rank == 0) {
        full_image.resize(width * height);
        full_output.resize(width * height);

        for (auto& p : full_image)
            p = static_cast<uint8_t>(rand() % 256);

        std::cout << "Running MPI Median Filter on "
                  << size << " processes\n";
    }

    // Allocate local buffers (+ halos)
    std::vector<uint8_t> local_image((rows_per_rank + 2 * r) * width);
    std::vector<uint8_t> local_output((rows_per_rank + 2 * r) * width);

    // Scatter image rows
    MPI_Scatter(rank == 0 ? full_image.data() : nullptr,
                rows_per_rank * width,
                MPI_UNSIGNED_CHAR,
                local_image.data() + r * width,
                rows_per_rank * width,
                MPI_UNSIGNED_CHAR,
                0, MPI_COMM_WORLD);

    // Synchronize before timing
    MPI_Barrier(MPI_COMM_WORLD);
    double start_time = MPI_Wtime();

    // ---- HALO EXCHANGE ----
    if (rank > 0) {
        MPI_Sendrecv(local_image.data() + r * width,
                     r * width, MPI_UNSIGNED_CHAR,
                     rank - 1, 0,
                     local_image.data(),
                     r * width, MPI_UNSIGNED_CHAR,
                     rank - 1, 0,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    if (rank < size - 1) {
        MPI_Sendrecv(local_image.data() + rows_per_rank * width,
                     r * width, MPI_UNSIGNED_CHAR,
                     rank + 1, 0,
                     local_image.data() + (rows_per_rank + r) * width,
                     r * width, MPI_UNSIGNED_CHAR,
                     rank + 1, 0,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // ---- COMPUTE ----
    median_filter_mpi(local_image, local_output,
                      width, rows_per_rank + 2 * r, k);

    // Gather results
    MPI_Gather(local_output.data() + r * width,
               rows_per_rank * width,
               MPI_UNSIGNED_CHAR,
               rank == 0 ? full_output.data() : nullptr,
               rows_per_rank * width,
               MPI_UNSIGNED_CHAR,
               0, MPI_COMM_WORLD);

    // Stop timing
    MPI_Barrier(MPI_COMM_WORLD);
    double end_time = MPI_Wtime();

    if (rank == 0) {
        std::cout << "Filtering complete.\n";
        std::cout << "Parallel Time (MPI): "
                  << (end_time - start_time) << " s\n";
    }

    MPI_Finalize();
    return 0;
}
