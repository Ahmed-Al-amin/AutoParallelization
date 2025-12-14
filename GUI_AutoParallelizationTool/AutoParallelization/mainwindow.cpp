#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QProcess>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QDebug>
#include <QDir>

// --- CONFIGURATION ---
// Ensure these paths match your PC exactly
const QString MPI_INC = "-I\"C:\\Program Files (x86)\\Microsoft SDKs\\MPI\\Include\"";
const QString MPI_LIB = "-L\"C:\\Program Files (x86)\\Microsoft SDKs\\MPI\\Lib\\x64\"";
const QString MPI_RUN = "\"C:\\Program Files\\Microsoft MPI\\Bin\\mpiexec.exe\"";

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->inputCode->setPlaceholderText("Enter Sequential Code Here");
}

MainWindow::~MainWindow()
{
    delete ui;
}

// --- HELPER: Extract Values using Regex ---
QString extractValue(const QString &code, QString varName) {
    // Looks for patterns like: int width = 4096;
    QRegularExpression re("int\\s+" + varName + "\\s*=\\s*([0-9*]+)\\s*;");
    QRegularExpressionMatch match = re.match(code);
    if (match.hasMatch()) {
        return match.captured(1);
    }
    return "0";
}

void MainWindow::on_runButton_clicked()
{
    ui->outputLog->clear();
    ui->outputLog->append(">>> Starting Auto-Parallelization...");

    // 1. GET INPUTS FROM GUI
    QString seqCode = ui->inputCode->toPlainText();
    int numProcs = ui->procCount->value();

    if (seqCode.isEmpty()) {
        ui->outputLog->append("[Error] No sequential code entered!");
        return;
    }

    // 2. PARSE PARAMETERS
    QString w = extractValue(seqCode, "width");
    QString h = extractValue(seqCode, "height");
    QString k = extractValue(seqCode, "k");

    // Defaults
    if (w == "0") { w = "4096"; h = "4096"; }
    if (k == "0") k = "5";

    ui->outputLog->append("[Tool] Detected Config: " + w + "x" + h + " (k=" + k + ")");

    // 3. GENERATE PARALLEL CODE
    // We construct the parallel C++ code string here
    QString parCode = R"(
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

    // --- INJECTED VALUES ---
    int width = %1;
    int height = %2;
    int k = %3;

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
    // Inject variables into the string
    parCode = parCode.arg(w, h, k);

    // Display it in the UI
    ui->parallelCode->setPlainText(parCode);

    // 4. SAVE TO FILE
    QString buildPath = QDir::currentPath(); // Save where the app is running
    QString fileName = buildPath + "/_gui_gen_parallel.cpp";
    QString exeName = buildPath + "/_gui_gen_parallel.exe";

    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << parCode;
        file.close();
    } else {
        ui->outputLog->append("[Error] Could not write file to disk.");
        return;
    }

    // 5. COMPILE
    ui->outputLog->append(">>> Compiling...");

    // Note: We use the system g++ command
    // We must quote paths carefully because of spaces in "Program Files"
    QString compileCmd = QString("g++ \"%1\" %2 %3 -lmsmpi -O2 -o \"%4\"")
                             .arg(fileName)
                             .arg(MPI_INC)
                             .arg(MPI_LIB)
                             .arg(exeName);

    QProcess compileProc;
    compileProc.start(compileCmd);
    compileProc.waitForFinished();

    if (compileProc.exitCode() != 0) {
        ui->outputLog->append("[Error] Compilation Failed:");
        ui->outputLog->append(compileProc.readAllStandardError());
        return;
    }
    ui->outputLog->append("[Success] Compilation Complete.");

    // 6. RUN
    ui->outputLog->append(QString(">>> Running on %1 processors...").arg(numProcs));

    QString runCmd = QString("%1 -n %2 \"%3\"")
                         .arg(MPI_RUN)
                         .arg(numProcs)
                         .arg(exeName);

    QProcess runProc;
    runProc.start(runCmd);
    runProc.waitForFinished();

    // Show Output
    QString output = runProc.readAllStandardOutput();
    QString error = runProc.readAllStandardError();

    ui->outputLog->append(output);
    if (!error.isEmpty()) {
        ui->outputLog->append("[StdErr]: " + error);
    }

    // Cleanup (optional)
    QFile::remove(fileName);
    QFile::remove(exeName);
}

void MainWindow::on_spinBox_textChanged(const QString &arg1)
{

}

