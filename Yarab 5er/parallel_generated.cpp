
    #include <mpi.h>
    #include <vector>
    #include <algorithm>
    #include <iostream>
    #include <cstdint>
    #include <cstdlib>

    void median_filter_kernel(const std::vector<uint8_t>& local_image, 
                              std::vector<uint8_t>& local_output,
                              int width, int rows, int k, int r) {
        std::vector<uint8_t> window;
        window.reserve(k * k);
        for (int y = 0; y < rows; ++y) {
            for (int x = 0; x < width; ++x) {
                window.clear();
                int center_y = y + r; 
                for (int dy = -r; dy <= r; ++dy) {
                    int ny = center_y + dy;
                    for (int dx = -r; dx <= r; ++dx) {
                        int nx = x + dx;
                        if (nx < 0 || nx >= width) continue; 
                        window.push_back(local_image[ny * width + nx]);
                    }
                }
                std::sort(window.begin(), window.end());
                local_output[y * width + x] = window[window.size() / 2];
            }
        }
    }

    int main(int argc, char* argv[]) {
        MPI_Init(NULL, NULL);
        int rank, size;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &size);

        int width = 4096;
        int height = 4096;
        int k = 5;
        int r = k / 2;
        int rows_per_proc = height / size;

        std::vector<uint8_t> full_image;
        std::vector<uint8_t> final_output;

        if (rank == 0) {
            full_image.resize(width * height);
            std::srand(12345);
            for (auto& p : full_image) p = static_cast<uint8_t>(rand() % 256);
            final_output.resize(width * height);
        }

        std::vector<uint8_t> my_chunk(rows_per_proc * width);
        MPI_Scatter(full_image.data(), rows_per_proc * width, MPI_UNSIGNED_CHAR,
                    my_chunk.data(), rows_per_proc * width, MPI_UNSIGNED_CHAR,
                    0, MPI_COMM_WORLD);

        MPI_Barrier(MPI_COMM_WORLD);
        double t0 = MPI_Wtime();

        std::vector<uint8_t> padded_chunk((rows_per_proc + 2 * r) * width, 0);
        std::copy(my_chunk.begin(), my_chunk.end(), padded_chunk.begin() + (r * width));

        MPI_Request reqs[4];
        int n_reqs = 0;

        if (rank > 0) {
            MPI_Irecv(padded_chunk.data(), r * width, MPI_UNSIGNED_CHAR, rank - 1, 0, MPI_COMM_WORLD, &reqs[n_reqs++]);
            MPI_Isend(padded_chunk.data() + r * width, r * width, MPI_UNSIGNED_CHAR, rank - 1, 1, MPI_COMM_WORLD, &reqs[n_reqs++]);
        }
        if (rank < size - 1) {
            MPI_Isend(padded_chunk.data() + rows_per_proc * width, r * width, MPI_UNSIGNED_CHAR, rank + 1, 0, MPI_COMM_WORLD, &reqs[n_reqs++]);
            MPI_Irecv(padded_chunk.data() + (rows_per_proc + r) * width, r * width, MPI_UNSIGNED_CHAR, rank + 1, 1, MPI_COMM_WORLD, &reqs[n_reqs++]);
        }
        MPI_Waitall(n_reqs, reqs, MPI_STATUSES_IGNORE);

        std::vector<uint8_t> my_output(rows_per_proc * width);
        median_filter_kernel(padded_chunk, my_output, width, rows_per_proc, k, r);

        MPI_Gather(my_output.data(), rows_per_proc * width, MPI_UNSIGNED_CHAR,
                   final_output.data(), rows_per_proc * width, MPI_UNSIGNED_CHAR,
                   0, MPI_COMM_WORLD);

        double t1 = MPI_Wtime();
        if (rank == 0) {
            std::cout << "[Parallel] Success! Time: " << (t1 - t0) << " s" << std::endl;
        }

        MPI_Finalize();
        return 0;
    }
    