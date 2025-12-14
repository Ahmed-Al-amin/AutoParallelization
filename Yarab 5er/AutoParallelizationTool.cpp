#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <cstdio>  // For remove()
#include <cstdlib> // For system()

using namespace std;

// --- CONFIGURATION ---
// Ensure these paths match your system
const string MPI_INC = "-I\"C:\\Program Files (x86)\\Microsoft SDKs\\MPI\\Include\"";
const string MPI_LIB = "-L\"C:\\Program Files (x86)\\Microsoft SDKs\\MPI\\Lib\\x64\"";
const string MPI_RUN = "\"C:\\Program Files\\Microsoft MPI\\Bin\\mpiexec.exe\"";

// --- HELPER: EXTRACT INPUTS ---
string extract_value(const string& source_code, string var_name) {
    regex pattern("int\\s+" + var_name + "\\s*=\\s*([0-9*]+)\\s*;");
    smatch match;
    if (regex_search(source_code, match, pattern)) return match[1];
    return "0";
}

// --- THE CORE FUNCTION ---
// Now accepts 'num_procs' as an argument
void run_parallel_version(string width, string height, string k, string num_procs) {
    
    cout << "\n[System] Preparing Parallel Run (" << width << "x" << height << ") on " << num_procs << " processors..." << std::endl;

    // 1. Define the Parallel Source Code
    string code = R"(
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

        int width = )" + width + R"(;
        int height = )" + height + R"(;
        int k = )" + k + R"(;
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
    )";

    string src = "parallel_generated.cpp";
    string exe = "parallel_generated.exe";

    ofstream out(src);
    out << code;
    out.close();

    string cmd_build = "g++ " + src + " " + MPI_INC + " " + MPI_LIB + " -lmsmpi -O2 -o " + exe;
    if (system(cmd_build.c_str()) != 0) {
        cerr << "[Error] Compilation failed. Check " << src << " for errors." << endl;
        return;
    }

    // 3. Execution 
    cout << "-----------------------------------" << endl;
    string cmd_run = MPI_RUN + " -n " + num_procs + " " + exe;
    system(cmd_run.c_str());
    cout << "-----------------------------------" << endl;
}

int main() {
    cout << "===========================================" << endl;
    cout << "   AUTO-PARALLELIZER TOOL" << endl;
    cout << "===========================================" << endl;

    string filename;
    cout << "1. Enter sequential file name: ";
    cin >> filename;

    string num_procs;
    cout << "2. Enter number of processors to use: ";
    cin >> num_procs;

    ifstream file(filename);
    if (!file) {
        cerr << "File '" << filename << "' not found!" << endl;
        return 1;
    }
    string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    string w = extract_value(content, "width");
    string h = extract_value(content, "height");
    string k = extract_value(content, "k");

    if (w == "0") { w = "4096"; h = "4096"; } 
    if (k == "0") k = "5";

    cout << "[Tool] Configuration detected: " << w << "x" << h << endl;

    run_parallel_version(w, h, k, num_procs);

    cout << "[Tool] Done." << endl;
    return 0;
}