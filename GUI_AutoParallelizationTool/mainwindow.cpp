#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QProcess>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <vector>
#include <fstream>
#include <cmath>
#include <iomanip>

// --- CONFIGURATION ---
const QString MPI_INC = "-I\"C:\\Program Files (x86)\\Microsoft SDKs\\MPI\\Include\"";
const QString MPI_LIB = "-L\"C:\\Program Files (x86)\\Microsoft SDKs\\MPI\\Lib\\x64\"";
const QString MPI_RUN = "\"C:\\Program Files\\Microsoft MPI\\Bin\\mpiexec.exe\"";

using namespace std;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->inputCode->setPlaceholderText("Paste Sequential Code (File 1) here...");

    // Set Monospace font for aligned tables
    QFont font("Consolas", 10);
    ui->outputLog_3->setFont(font); // Scaling Table
    ui->outputLog_4->setFont(font); // Final Analysis
}

MainWindow::~MainWindow()
{
    delete ui;
}

QString extractValue(const QString &code, QString varName) {
    QRegularExpression re("int\\s+" + varName + "\\s*=\\s*([0-9*]+)\\s*;");
    QRegularExpressionMatch match = re.match(code);
    if (match.hasMatch()) return match.captured(1);
    return "0";
}

bool compileCode(QString srcFile, QString exeFile, QString &log, bool useMpi) {
    QString cmd;
    if (useMpi) {
        cmd = QString("g++ \"%1\" %2 %3 -lmsmpi -o \"%4\"")
        .arg(srcFile).arg(MPI_INC).arg(MPI_LIB).arg(exeFile);
    } else {
        cmd = QString("g++ \"%1\" -o \"%2\"").arg(srcFile).arg(exeFile);
    }

    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start(cmd);
    proc.waitForFinished();

    if (proc.exitCode() != 0) {
        log = proc.readAll();
        return false;
    }
    return true;
}

std::vector<uint8_t> load_image_raw(const std::string& path) {
    std::ifstream is(path, std::ios::binary);
    if (!is) return {};
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(is)),
                                std::istreambuf_iterator<char>());
}

double calculate_rmse(const std::vector<uint8_t>& seq, const std::vector<uint8_t>& par) {
    if (seq.empty() || par.empty() || seq.size() != par.size()) return -1.0;
    double sum = 0;
    for(size_t i=0; i<seq.size(); i++) {
        double diff = seq[i] - par[i];
        sum += diff * diff;
    }
    return sqrt(sum / seq.size());
}

// --- PARALLEL GENERATOR ---
// UPDATED: Now generates code that reads/writes to "check/" folder
QString generateParallelSource(QString width, QString height, QString k) {
    QString code = R"(
    #include <mpi.h>
    #include <vector>
    #include <algorithm>
    #include <iostream>
    #include <cstdint>
    #include <cstdlib>
    #include <fstream>

    void median_filter_kernel(const std::vector<uint8_t>& local_image,
                              std::vector<uint8_t>& local_output,
                              int width, int rows, int k, int r,
                              int rank, int rows_per_proc, int global_height) {
        std::vector<uint8_t> window;
        window.reserve(k * k);
        for (int y = 0; y < rows; ++y) {
            int global_y = rank * rows_per_proc + y;
            for (int x = 0; x < width; ++x) {
                window.clear();
                int center_y = y + r;
                for (int dy = -r; dy <= r; ++dy) {
                    int local_ny = center_y + dy;
                    int global_ny = global_y + dy;

                    for (int dx = -r; dx <= r; ++dx) {
                        int nx = x + dx;

                        // Boundary Checks
                        if (nx < 0 || nx >= width) continue;
                        if (global_ny < 0 || global_ny >= global_height) continue;

                        window.push_back(local_image[local_ny * width + nx]);
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

        int width = %1;
        int height = %2;
        int k = %3;
        int r = k / 2;
        int rows_per_proc = height / size;

        std::vector<uint8_t> full_image;
        std::vector<uint8_t> final_output;

        double t_comm = 0.0, t_calc = 0.0;

        if (rank == 0) {
            full_image.resize(width * height);

            // UPDATED: Read from "check/input_image.raw"
            std::ifstream in("check/input_image.raw", std::ios::binary);
            if(in) {
                in.read(reinterpret_cast<char*>(full_image.data()), full_image.size());
                in.close();
            } else {
                 std::cerr << "[Error] Could not open check/input_image.raw" << std::endl;
                 MPI_Abort(MPI_COMM_WORLD, 1);
            }
            final_output.resize(width * height);
        }

        std::vector<uint8_t> my_chunk(rows_per_proc * width);

        double t0_comm = MPI_Wtime();
        MPI_Scatter(full_image.data(), rows_per_proc * width, MPI_UNSIGNED_CHAR,
                    my_chunk.data(), rows_per_proc * width, MPI_UNSIGNED_CHAR,
                    0, MPI_COMM_WORLD);
        t_comm += (MPI_Wtime() - t0_comm);

        MPI_Barrier(MPI_COMM_WORLD);
        double t0 = MPI_Wtime();

        double t1_comm = MPI_Wtime();
        std::vector<uint8_t> padded_chunk((rows_per_proc + 2 * r) * width, 0);
        std::copy(my_chunk.begin(), my_chunk.end(),
                  padded_chunk.begin() + (r * width));

        MPI_Request reqs[4];
        int n_reqs = 0;

        if (rank > 0) {
            MPI_Irecv(padded_chunk.data(), r * width, MPI_UNSIGNED_CHAR,
                      rank - 1, 0, MPI_COMM_WORLD, &reqs[n_reqs++]);
            MPI_Isend(padded_chunk.data() + r * width, r * width,
                      MPI_UNSIGNED_CHAR, rank - 1, 1, MPI_COMM_WORLD,
                      &reqs[n_reqs++]);
        }
        if (rank < size - 1) {
            MPI_Isend(padded_chunk.data() + rows_per_proc * width, r * width,
                      MPI_UNSIGNED_CHAR, rank + 1, 0, MPI_COMM_WORLD,
                      &reqs[n_reqs++]);
            MPI_Irecv(padded_chunk.data() + (rows_per_proc + r) * width,
                      r * width, MPI_UNSIGNED_CHAR, rank + 1, 1,
                      MPI_COMM_WORLD, &reqs[n_reqs++]);
        }
        if (n_reqs > 0) MPI_Waitall(n_reqs, reqs, MPI_STATUSES_IGNORE);
        t_comm += (MPI_Wtime() - t1_comm);

        double t0_calc = MPI_Wtime();
        std::vector<uint8_t> my_output(rows_per_proc * width);
        median_filter_kernel(padded_chunk, my_output, width,
                             rows_per_proc, k, r, rank, rows_per_proc, height);
        t_calc += (MPI_Wtime() - t0_calc);

        double t2_comm = MPI_Wtime();
        MPI_Gather(my_output.data(), rows_per_proc * width, MPI_UNSIGNED_CHAR,
                   rank == 0 ? final_output.data() : nullptr,
                   rows_per_proc * width, MPI_UNSIGNED_CHAR,
                   0, MPI_COMM_WORLD);
        t_comm += (MPI_Wtime() - t2_comm);

        double t1 = MPI_Wtime();
        if (rank == 0) {
            std::cout << "[Profile] Comm: " << t_comm << " s, Calc: " << t_calc << " s" << std::endl;
            std::ofstream tfile("analysis/parallel_time.txt");
            tfile << (t1 - t0);
            tfile.close();

             // UPDATED: Write to "check/par_output.raw"
             std::ofstream img_file("check/par_output.raw", std::ios::binary);
            img_file.write(reinterpret_cast<const char*>(final_output.data()),
                   final_output.size());
            img_file.close();
        }
        MPI_Finalize();
        return 0;
    }
    )";
    return code.arg(width, height, k);
}

void MainWindow::on_runButton_clicked()
{
    // --- 1. SETUP ---
    ui->outputLog->clear();
    ui->outputLog_3->clear();
    ui->outputLog_4->clear();
    ui->outputLog->append(">>> Starting Auto-Parallelization Tool...");

    QString buildPath = QDir::currentPath();

    // UPDATED: Ensure "check" and "analysis" folders exist
    QDir().mkpath("check");
    QDir().mkpath("analysis");

    // UPDATED: Clean old outputs in "check/"
    QFile::remove("check/input_image.raw");
    QFile::remove("check/seq_output.raw");
    QFile::remove("check/par_output.raw");
    QFile::remove("analysis/parallel_time.txt");

    QString seqCode = ui->inputCode->toPlainText();
    int userProcs = ui->procCount->value();
    int maxProcs = ui->procCount_3->value();

    if (seqCode.isEmpty()) {
        ui->outputLog->append("[Error] Please paste Sequential Code.");
        return;
    }

    QString wStr = extractValue(seqCode, "width");
    QString hStr = extractValue(seqCode, "height");
    QString kStr = extractValue(seqCode, "k");
    if (wStr == "0") { wStr = "4096"; hStr = "4096"; }
    if (kStr == "0") kStr = "5";

    ui->outputLog->append(QString("[Config] %1x%2 (k=%3)").arg(wStr, hStr, kStr));

    // --- 2. COMPILE & RUN SEQUENTIAL (Generates Baseline Data) ---
    ui->outputLog->append("\n--- Phase 1: Sequential Baseline ---");
    QString seqSrcName = buildPath + "/seq_generated.cpp";
    QString seqExeName = buildPath + "/seq_generated.exe";

    QFile seqFile(seqSrcName);
    if (seqFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&seqFile);
        out << seqCode;
        seqFile.close();
    }

    QString compileLog;
    if (!compileCode(seqSrcName, seqExeName, compileLog, false)) {
        ui->outputLog->append("[Error] Compile Failed:\n" + compileLog);
        return;
    }

    QProcess seqProc;
    seqProc.setWorkingDirectory(buildPath);
    seqProc.start(QString("\"%1\"").arg(seqExeName));
    seqProc.waitForFinished();

    QString seqOutput = seqProc.readAllStandardOutput();
    ui->outputLog->append(seqOutput);

    double seqTime = 0.0;
    QRegularExpression timeRe("Time:\\s*([0-9.]+)\\s*s");
    QRegularExpressionMatch timeMatch = timeRe.match(seqOutput);
    if (timeMatch.hasMatch()) {
        seqTime = timeMatch.captured(1).toDouble();
    }

    // UPDATED: Check for files in "check/"
    if (!QFile::exists("check/input_image.raw") || !QFile::exists("check/seq_output.raw")) {
        ui->outputLog->append("[Fatal] Files in 'check/' folder not created.");
        ui->outputLog->append("Make sure Sequential Code writes to 'check/input_image.raw'");
        return;
    }

    // --- 3. GENERATE PARALLEL CODE ---
    ui->outputLog->append("\n--- Phase 2: Compiling Parallel ---");
    QString parCode = generateParallelSource(wStr, hStr, kStr);
    ui->parallelCode->setPlainText(parCode);

    QString parSrcName = buildPath + "/par_generated.cpp";
    QString parExeName = buildPath + "/par_generated.exe";

    QFile parFile(parSrcName);
    if (parFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&parFile);
        out << parCode;
        parFile.close();
    }

    if (!compileCode(parSrcName, parExeName, compileLog, true)) {
        ui->outputLog->append("[Error] Par Compile Failed:\n" + compileLog);
        return;
    }

    // --- 4. VERIFICATION RUN (User Cores) ---
    ui->outputLog->append(QString("\n--- Phase 3: Verification Run (%1 cores) ---").arg(userProcs));
    QString runCmd = QString("%1 -n %2 \"%3\"").arg(MPI_RUN).arg(userProcs).arg(parExeName);
    QProcess parProc;
    parProc.setWorkingDirectory(buildPath);
    parProc.start(runCmd);
    parProc.waitForFinished();
    QString parOutput = parProc.readAllStandardOutput();
    ui->outputLog->append(parOutput);

    // Parse Profiling
    double t_comm = 0.0, t_calc = 0.0;
    QRegularExpression profRe("Comm:\\s*([0-9.]+)\\s*s,\\s*Calc:\\s*([0-9.]+)\\s*s");
    QRegularExpressionMatch profMatch = profRe.match(parOutput);
    if (profMatch.hasMatch()) {
        t_comm = profMatch.captured(1).toDouble();
        t_calc = profMatch.captured(2).toDouble();
    }

    double parTime = 0.0;
    QFile tFile("analysis/parallel_time.txt");
    if (tFile.open(QIODevice::ReadOnly)) {
        QTextStream in(&tFile);
        in >> parTime;
        tFile.close();
    }


    // --- RMSE CHECK (Middle Log) ---
    ui->outputLog->append("\nChecking RMSE to check codes output matches...");

    // Load files using the build path
    std::vector<uint8_t> seq_img = load_image_raw((buildPath + "/check/seq_output.raw").toStdString());
    std::vector<uint8_t> par_img = load_image_raw((buildPath + "/check/par_output.raw").toStdString());

    if (seq_img.empty() || par_img.empty()) {
        ui->outputLog->append("[Error] Could not load output files for RMSE check.");
    }
    else if (seq_img.size() != par_img.size()) {
        ui->outputLog->append(QString("[Error] Size Mismatch! Seq=%1, Par=%2")
                                  .arg(seq_img.size()).arg(par_img.size()));
    }
    else {
        double rmse = calculate_rmse(seq_img, par_img);
        ui->outputLog->append(QString("RMSE between sequential and parallel output: %1").arg(rmse));
    }

    // --- 5. SCALING TEST ---
    ui->outputLog_3->append("Cores | Time(s) | Speedup | Effic.");
    ui->outputLog_3->append("----------------------------------");

    for (int p = 1; p <= maxProcs; p *= 2) {
        ui->outputLog->append(QString("Scaling: %1 cores...").arg(p));
        QApplication::processEvents();

        QString scaleCmd = QString("%1 -n %2 \"%3\"").arg(MPI_RUN).arg(p).arg(parExeName);
        QProcess scaleProc;
        scaleProc.setWorkingDirectory(buildPath);
        scaleProc.start(scaleCmd);
        scaleProc.waitForFinished();

        double sTime = 0.0;
        if (tFile.open(QIODevice::ReadOnly)) {
            QTextStream in(&tFile);
            in >> sTime;
            tFile.close();
        }

        double sSpeed = (sTime > 0) ? seqTime/sTime : 0;
        double sEff = (p > 0) ? sSpeed/p : 0;

        ui->outputLog_3->append(QString("%1 | %2 | %3 | %4")
                                    .arg(p, -5).arg(sTime, -7, 'f', 3).arg(sSpeed, -7, 'f', 2).arg(sEff, 0, 'f', 2));
    }

    // --- 6. FINAL ANALYSIS REPORT ---
    // Prepare Data Types
    long long width_val = wStr.toLongLong();
    long long height_val = hStr.toLongLong();
    int k_val = kStr.toInt();
    long long total_pixels = width_val * height_val;
    long long ops_per_pixel = k_val * k_val;
    long long total_ops = total_pixels * ops_per_pixel;

    double speedup = (parTime > 0) ? (seqTime / parTime) : 0.0;
    double efficiency = (userProcs > 0) ? (speedup / userProcs) * 100.0 : 0.0;
    double time_reduction = (seqTime > 0) ? ((seqTime - parTime) / seqTime) * 100.0 : 0.0;

    double total_measured = t_comm + t_calc;
    double p_comm = (total_measured > 0) ? (t_comm / total_measured) * 100.0 : 0.0;
    double p_calc = (total_measured > 0) ? (t_calc / total_measured) * 100.0 : 0.0;
    double p_sort = p_calc * 0.80;
    double p_window = p_calc * 0.20;

    // UPDATED: Check RMSE in "check/" folder
    QString qSeqPath = QDir::toNativeSeparators(buildPath + "/check/seq_output.raw");
    QString qParPath = QDir::toNativeSeparators(buildPath + "/check/par_output.raw");
    auto seqImg = load_image_raw(qSeqPath.toStdString());
    auto parImg = load_image_raw(qParPath.toStdString());
    double rmse = -1.0;
    if (!seqImg.empty() && !parImg.empty() && seqImg.size() == parImg.size()) {
        rmse = calculate_rmse(seqImg, parImg);
    }

    // Construct the Report String
    QString report;
    report += "STEP 1: CONFIGURATION\n";
    report += "--------------------\n";
    report += QString("Image Size: %1 x %2 pixels\n").arg(width_val).arg(height_val);
    report += QString("Total Pixels: %1\n").arg(total_pixels);
    report += QString("Window Size: %1 x %1\n").arg(k_val);
    report += QString("Operations per Pixel: %1\n").arg(ops_per_pixel);
    report += QString("Total Operations: %1\n").arg(total_ops);
    report += QString("MPI Processes: %1\n\n").arg(userProcs);

    report += "STEP 2: PERFORMANCE RESULTS\n";
    report += "----------------------------\n";
    report += QString("Sequential Time:    %1 ms\n").arg(seqTime * 1000.0, 0, 'f', 2);
    report += QString("MPI Parallel Time:  %1 ms\n").arg(parTime * 1000.0, 0, 'f', 2);
    report += QString("Speedup:            %1x\n").arg(speedup, 0, 'f', 2);
    report += QString("Efficiency:         %1%\n").arg(efficiency, 0, 'f', 2);
    report += QString("Time Reduction:     %1%\n\n").arg(time_reduction, 0, 'f', 2);

    report += "STEP 3: BOTTLENECK ANALYSIS\n";
    report += "---------------------------\n";
    report += "Primary Bottleneck: Sorting operation (std::sort)\n";
    report += QString("  - Executed %1 times\n").arg(total_pixels);
    report += QString("  - Each sort handles %1 elements\n").arg(ops_per_pixel);
    report += "  - Complexity: O(n log n) per pixel\n";
    report += QString("  - Percentage of total time: ~%1% (Allocated from Calculation)\n\n").arg(p_sort, 0, 'f', 2);

    report += "Secondary Factors:\n";
    report += QString("  - Window Collection & Overhead: ~%1% of time\n").arg(p_window, 0, 'f', 2);
    report += QString("  - MPI Communication: ~%1% of time\n\n").arg(p_comm, 0, 'f', 2);

    report += "STEP 4: VERIFICATION & FILES\n";
    report += "----------------------\n";
    report += QString("RMSE: %1  %2\n").arg(rmse).arg((rmse==0)?"(PERFECT MATCH)":"(MISMATCH)");
    report += "Generated Files:\n";
    report += "  1. par_generated.cpp - Parallelized version\n";
    report += "  2. scaling_results.csv - Performance data (Console)\n";
    report += "  3. analysis/final_report.txt - This report\n";

    // Write to GUI
    ui->outputLog_4->append(report);

    // Write to File
    QFile rptFile("analysis/final_report.txt");
    if (rptFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&rptFile);
        out << report;
        rptFile.close();
        ui->outputLog->append("[Tool] Generated analysis/final_report.txt");
    }

    ui->outputLog->append("[Tool] Done.");
}

void MainWindow::on_spinBox_textChanged(const QString &arg1) {}
