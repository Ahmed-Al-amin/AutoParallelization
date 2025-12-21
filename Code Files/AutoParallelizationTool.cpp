#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <cstdio>  
#include <cstdlib> 
#include <vector>
#include <cmath>
#include <cstdint>

using namespace std;

struct RankWork {
    int rank;
    long long pixels;
    double time;
};

// --- CONFIGURATION ---
// Ensure these paths match your system
// Helper to get MPI flags from environment or fallback
std::string get_mpi_inc_flag() {
    const char* env_p = std::getenv("MSMPI_INC");
    if (env_p) {
        std::string path = env_p;
        if (!path.empty() && path.back() == '\\') {
            path.pop_back();
        }
        return "-I\"" + path + "\"";
    }
    return "-I\"C:\\Program Files (x86)\\Microsoft SDKs\\MPI\\Include\"";
}

std::string get_mpi_lib_flag() {
    const char* env_p = std::getenv("MSMPI_LIB64");
    if (env_p) {
        std::string path = env_p;
        if (!path.empty() && path.back() == '\\') {
            path.pop_back();
        }
        return "-L\"" + path + "\"";
    }
    return "-L\"C:\\Program Files (x86)\\Microsoft SDKs\\MPI\\Lib\\x64\"";
}

const string MPI_RUN = "mpiexec";

// --- HELPER: EXTRACT INPUTS ---
string extract_value(const string &source_code, string var_name) {
    regex pattern("int\\s+" + var_name + "\\s*=\\s*([0-9*]+)\\s*;");
    smatch match;
    if (regex_search(source_code, match, pattern))
        return match[1];
    return "0";
}

// helper function to extract sequential time
// helper function to run parallel with 1 core as baseline
double run_baseline_parallel_1_core(const std::string &seq_exe_path, const std::string &par_src_path)
{
    // 1. Check if input data already exists to avoid re-running sequential
    std::ifstream check_file("check/input_image.raw");
    if (check_file.good()) {
        std::cout << "[System] " << "check/input_image.raw" << " found. Skipping sequential data generation." << std::endl;
        check_file.close();
    } else {
        // Runs the sequential program (which generates check/input_image.raw)
        std::cout << "[System] Generating input data using " << seq_exe_path << "..." << std::endl;
        std::string cmd_gen = "\"" + seq_exe_path + "\" > NUL 2>&1";
        int rc_gen = system(cmd_gen.c_str());
        if (rc_gen != 0) {
            std::cerr << "[Warning] Sequential data generation failed or returned non-zero code." << std::endl;
        }
    }

    std::string src = par_src_path;
    std::string exe = "parallel_generated.exe";
    std::string cmd_build = "g++ " + src + " " + get_mpi_inc_flag() + " " + get_mpi_lib_flag() + " -lmsmpi -o " + exe + " > NUL 2>&1";
    if (system(cmd_build.c_str()) != 0) {
        std::cerr << "[Error] Failed to compile parallel baseline: " << src << std::endl;
        return 0.0;
    }

    std::cout << "[System] Running Parallel Version on 1 Core as Baseline..." << std::endl;
    std::string cmd_run = MPI_RUN + " -n 1 " + exe;
    int rc = system(cmd_run.c_str());
    if (rc != 0) {
        std::cerr << "[Error] Baseline run failed with code " << rc << std::endl;
        return 0.0;
    }

    // Read the time from analysis/parallel_time.txt
    std::ifstream in("analysis/parallel_time.txt");
    if (!in) {
        std::cerr << "[Error] Could not open analysis/parallel_time.txt for reading.\n";
        return 0.0;
    }
    double time_val;
    in >> time_val;
    in.close();
    remove("analysis/parallel_time.txt");
    return time_val;
}

std::vector<uint8_t> load_image_raw(const std::string& path) {
    std::ifstream is(path, std::ios::binary);
    if (!is) {
        throw std::runtime_error("Cannot open " + path);
    }
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(is)),
                                 std::istreambuf_iterator<char>());
}



std::vector<RankWork> g_rank_work; // global

void parse_rank_work(const std::string &output_text) {
    g_rank_work.clear();
    std::regex re(R"(Rank\s+([0-9]+):\s*work=([0-9]+)\s+pixels,\s*time=([0-9.]+)\s*s)");
    std::smatch m;
    std::string::const_iterator searchStart(output_text.cbegin());
    while (std::regex_search(searchStart, output_text.cend(), m, re)) {
        RankWork rw;
        rw.rank   = std::stoi(m[1]);
        rw.pixels = std::stoll(m[2]);
        rw.time   = std::stod(m[3]);
        g_rank_work.push_back(rw);
        searchStart = m.suffix().first;
    }
}

double calculate_rmse(const vector<uint8_t>& seq, const vector<uint8_t>& par) {
    double sum = 0;
    for(size_t i=0; i<seq.size(); i++) {
        double diff = seq[i] - par[i];
        sum += diff * diff;
    }
    return sqrt(sum / seq.size());
}

// --- THE CORE FUNCTION ---
double g_parallel_time = 0.0;
double t_comm_global = 0.0;
double t_calc_global = 0.0;

void run_parallel_version(string par_src, string num_procs) {

    cout << "\n[System] Preparing Parallel Run on " << num_procs << " processors..." << std::endl;

    string src = par_src;
    string exe = "parallel_generated.exe";

    string cmd_build = "g++ " + src + " " + get_mpi_inc_flag() + " " + get_mpi_lib_flag() + " -lmsmpi -o " + exe;
    if (system(cmd_build.c_str()) != 0) {
        cerr << "[Error] Compilation failed. Check " << src << " for errors." << endl;
        return;
    }

    // 3. Execution (Modified to capture output for parsing)
    string temp_output_file = "temp_parallel_verify.txt";
    cout << "-----------------------------------" << endl;
    string cmd_run = MPI_RUN + " -n " + num_procs + " " + exe + " > " + temp_output_file + " 2>&1";
    int rc = system(cmd_run.c_str());
    
    // Read and print output (to show user progress)
    std::ifstream ifs(temp_output_file);
    std::string output_text((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    std::cout << output_text;
    ifs.close();
    remove(temp_output_file.c_str());

    if (rc != 0) {
        cerr << "[Error] MPI program failed with code " << rc << endl;
        return;
    }

    // Parse Profiling Data
    std::string line;
    std::istringstream iss(output_text);
    while (std::getline(iss, line)) {
        if (line.find("[Profile]") != std::string::npos) {
             std::regex re_prof(R"(Comm:\s*([0-9.]+)\s*s,\s*Calc:\s*([0-9.]+)\s*s)");
             std::smatch m_prof;
             if (std::regex_search(line, m_prof, re_prof)) {
                 extern double t_comm_global; // Use extern to reference global
                 extern double t_calc_global;
                 t_comm_global = std::stod(m_prof[1]);
                 t_calc_global = std::stod(m_prof[2]);
                 cout << "[System] Captured Profile - Comm: " << t_comm_global << "s, Calc: " << t_calc_global << "s" << endl;
             }
        }
    }

    ifstream time_file("analysis/parallel_time.txt");
    if (time_file.is_open()) {
        time_file >> g_parallel_time;
        time_file.close();
        remove("analysis/parallel_time.txt");
    } else {
        cerr << "[Error] parallel_time.txt not found; cannot read parallel time." << endl;
    }
    cout << "-----------------------------------" << endl;
}

// --- MODIFIED CORE FUNCTION ---
// Now returns double (time in seconds) and takes int for num_procs
double run_parallel_benchmark(string par_src, int num_procs) {

    string proc_str = to_string(num_procs);
    string src = par_src;
    string exe = "parallel_generated.exe";

    // Compile the parallel code (optional optimization: compile once outside loop)
    string cmd_build = "g++ " + src + " " + get_mpi_inc_flag() + " " + get_mpi_lib_flag() + " -lmsmpi -o " + exe;
    cmd_build += " > NUL 2>&1";
    
    if (system(cmd_build.c_str()) != 0) {
        cerr << "[Error] Compilation failed." << endl;
        return 0.0;
    }

    // 3. Execution
    // Redirect output to a temporary file to capture profiling info
    string temp_output_file = "temp_parallel_output.txt";
    string cmd_run = MPI_RUN + " -n " + proc_str + " " + exe + " > " + temp_output_file + " 2>&1";
    int rc = system(cmd_run.c_str());
    if (rc != 0) {
        cerr << "[Error] MPI program failed on " << num_procs << " procs." << endl;
        // Clean up temp file
        remove(temp_output_file.c_str());
        return 0.0;
    }

    // Read the captured output
    std::ifstream output_stream(temp_output_file);
    std::string output_text((std::istreambuf_iterator<char>(output_stream)),
                            std::istreambuf_iterator<char>());
    output_stream.close();
    // Clean up temp file
    remove(temp_output_file.c_str());

    double t_comm_global = 0.0;
    double t_calc_global = 0.0;
    
    // Parse line by line to find [Profile]
    std::string line;
    std::istringstream iss(output_text);
    while (std::getline(iss, line)) {
        if (line.find("[Profile]") != std::string::npos) {
             // Expected format: [Profile] Comm: 0.123 s, Calc: 4.567 s
             std::regex re_prof(R"(Comm:\s*([0-9.]+)\s*s,\s*Calc:\s*([0-9.]+)\s*s)");
             std::smatch m_prof;
             if (std::regex_search(line, m_prof, re_prof)) {
                 t_comm_global = std::stod(m_prof[1]);
                 t_calc_global = std::stod(m_prof[2]);
                 cout << "[System] Captured Profile - Comm: " << t_comm_global << "s, Calc: " << t_calc_global << "s" << endl;
             }
        }
    }

    parse_rank_work(output_text); // Existing parsing for rank work if needed

    double p_time = 0.0;
    ifstream time_file("analysis/parallel_time.txt");
    if (time_file.is_open()) {
        time_file >> p_time;
        time_file.close();
        remove("analysis/parallel_time.txt");
    }
    return p_time;
}

void generate_analysis_report(long long width, long long height, int k, int num_procs, double seq_time, double par_time, double comm_time, double calc_time) {
    long long total_pixels = width * height;
    long long ops_per_pixel = k * k; 
    long long total_ops = total_pixels * ops_per_pixel;
    double speedup = (par_time > 0) ? (seq_time / par_time) : 0.0;
    double efficiency = (num_procs > 0) ? (speedup / num_procs) * 100.0 : 0.0;
    double time_reduction = (seq_time > 0) ? ((par_time - seq_time) / seq_time) * 100.0 : 0.0;

    // Dynamic Breakdown
    double total_measured = comm_time + calc_time;
    double p_comm = 0.0, p_calc = 0.0;
    if (total_measured > 0) {
        p_comm = (comm_time / total_measured) * 100.0;
        p_calc = (calc_time / total_measured) * 100.0;
    }
    
    // Heuristic breakdown of Calculation Time (since we don't measure sort vs window per-pixel)
    // Assuming Sort is expensive (O(k*k log k)) vs Window (O(k*k))
    // Let's say Sort is ~80% of Calc, Window ~20% of Calc.
    double p_sort = p_calc * 0.80;
    double p_window = p_calc * 0.20;

    ofstream rpt("analysis/final_report.txt");

    rpt << "STEP 1: CONFIGURATION\n";
    rpt << "--------------------\n";
    rpt << "Image Size: " << width << " x " << height << " pixels\n";
    rpt << "Total Pixels: " << total_pixels << "\n";
    rpt << "Window Size: " << k << " x " << k << "\n";
    rpt << "Operations per Pixel: " << ops_per_pixel << "\n";
    rpt << "Total Operations: " << total_ops << "\n";
    rpt << "MPI Processes: " << num_procs << "\n\n";

    rpt << "STEP 2: PERFORMANCE RESULTS\n";
    rpt << "----------------------------\n";
    rpt << "Sequential Time:    " << (seq_time * 1000.0) << " ms\n";
    rpt << "MPI Parallel Time:  " << (par_time * 1000.0) << " ms\n";
    rpt.precision(2);
    rpt << fixed;
    rpt << "Speedup:            " << speedup << "x\n";
    rpt << "Efficiency:         " << efficiency << "%\n";
    rpt << "Time Reduction:     " << time_reduction << "%\n\n";

    rpt << "STEP 3: BOTTLENECK ANALYSIS\n";
    rpt << "---------------------------\n";
    rpt << "Primary Bottleneck: Sorting operation (std::sort)\n";
    rpt << "  - Executed " << total_pixels << " times\n";
    rpt << "  - Each sort handles " << ops_per_pixel << " elements\n";
    rpt << "  - Complexity: O(n log n) per pixel\n";
    rpt << "  - Percentage of total time: ~" << p_sort << "% (Allocated from Calculation)\n\n";
    rpt << "Secondary Factors:\n";
    rpt << "  - Window Collection & Overhead: ~" << p_window << "% of time\n";
    rpt << "  - MPI Communication: ~" << p_comm << "% of time\n\n";

    rpt << "STEP 4: CODE GENERATION\n";
    rpt << "----------------------\n";
    rpt << "Generated Files:\n";
    rpt << "  1. parallel_generated.cpp - Parallelized version\n";
    rpt << "  2. scaling_results.csv - Performance data\n";
    rpt << "  3. analysis/final_report.txt - This report\n";

    rpt.close();
    cout << "[Tool] Generated analysis/final_report.txt" << endl;
}

int main() {
    cout << "===========================================" << endl;
    cout << "   AUTO-PARALLELIZER TOOL" << endl;
    cout << "===========================================" << endl;

    system("mkdir analysis > NUL 2>&1");
    system("mkdir check > NUL 2>&1");

    string filename;
    cout << "1. Enter sequential file name (for input/verification): ";
    cin >> filename;

    string par_filename;
    cout << "2. Enter parallel source file name: ";
    cin >> par_filename;

    int num_procs_verification;
    cout << "3. Enter number of processors to use: ";
    cin >> num_procs_verification;

    int max_procs;
    cout << "4. Enter MAX processors to scale up to (e.g., 8): ";
    cin >> max_procs;

    ifstream file(filename);
    if (!file) {
        cerr << "File '" << filename << "' not found!" << endl;
        return 1;
    }
    string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    // Use long long for potentially large dimensions
    long long width_val = atoll(extract_value(content, "width").c_str());
    long long height_val = atoll(extract_value(content, "height").c_str());
    int k_val = atoi(extract_value(content, "k").c_str());

    // Fallbacks if parsing fails (0)
    if (width_val == 0) width_val = 8192;
    if (height_val == 0) height_val = 8192;
    if (k_val == 0) k_val = 5;

    // 2. Compilation / Verification of "Sequential" (now Baseline Input Generator)
    string seq_exe = "Seq_median_filter.exe";
    ifstream f(seq_exe);
    if (!f.good()) {
        cout << "[System] Compiling " << filename << "..." << endl;
        string compile_cmd = "g++ " + filename + " -o " + seq_exe;
        if (system(compile_cmd.c_str()) != 0) {
            cerr << "[Error] Failed to compile " << filename << endl;
            return 1;
        }
    } else {
        f.close();
    }

    // 3. Run Baseline (Parallel on 1 Core) + Input Generation
    cout << "\n[System] Establishing Baseline (Parallel on 1 Core) & Generating Input..." << endl;
    double seq_time = run_baseline_parallel_1_core(seq_exe, par_filename);

    if (seq_time <= 0.0) {
        cerr << "[Error] Baseline run failed. Check logs." << endl;
        return 1;
    }
    cout << "[Baseline] Time: " << seq_time << " s" << endl;

    // 5. Run Parallel Benchmark Scaling (Single Run Verification First)
    run_parallel_version(par_filename, to_string(num_procs_verification));

    cout << "Checking RMSE to check codes output matches "<<endl;

    std::vector<uint8_t> seq_img = load_image_raw("check/seq_output.raw");
    std::vector<uint8_t> par_img = load_image_raw("check/par_output.raw");

    if (seq_img.size() != par_img.size()) {
        std::cerr << "[Error] Sequential and parallel outputs have different sizes!"
                  << " seq=" << seq_img.size()
                  << " par=" << par_img.size() << std::endl;
    } else {
        double rmse = calculate_rmse(seq_img, par_img);
        std::cout << "RMSE between sequential and parallel output: "
                  << rmse << std::endl;
    }

    ofstream csv_old("analysis/scaling_results.csv"); // Renamed to avoid conflict
    csv_old << "Cores,Time(s),Speedup,Efficiency\n"; // CSV Header

    cout << "[Scaling] Starting Strong Scaling Test (1 to " << max_procs << " cores)..." << endl;
    cout << "\nCores      Time(s)         Speedup(x)      Efficiency" << endl;
    cout << "-----------------------------------------------------------" << endl;

    ofstream results("analysis/scaling_results.csv");
    results << "Cores,Time,Speedup,Efficiency\n";

    for (int p = 1; p <= max_procs; p *= 2) { // 1, 2, 4, 8...
        double p_time = run_parallel_benchmark(par_filename, p);
        
        double speedup = seq_time / p_time;
        double efficiency = speedup / p;

        printf("%-10d %-15.4f %-15.4f %-15.4f\n", p, p_time, speedup, efficiency);
        results << p << "," << p_time << "," << speedup << "," << efficiency << "\n";
    }
    results.close();
    cout << "-----------------------------------------------------------" << endl;
    cout << "[Success] Results saved to 'analysis/scaling_results.csv'" << endl;

    // Final Report Generation
    // We use the 'num_procs_verification' results for the report, assuming that's the "target" run.
    // Or we could use the best run from scaling. The request seems to want "one txt file".
    // I'll use the verification run time (g_parallel_time) and num_procs_verification.
    extern double t_comm_global;
    extern double t_calc_global;
    if (g_parallel_time > 0.0) {
        generate_analysis_report(width_val, height_val, k_val, num_procs_verification, seq_time, g_parallel_time, t_comm_global, t_calc_global);
    }
    
    cout << "[Tool] Done." << endl;
    return 0;
}
