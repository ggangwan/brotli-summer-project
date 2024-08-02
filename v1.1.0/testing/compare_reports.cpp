#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <algorithm>

// Define your ReportRow structure or class here
struct ReportRow {
    std::string file_name;
    double original_size;
    double compression_size;
    double compression_time;
    double brotli_time;
    double cpu_usage;
    int compression_quality; // Example member for compression level
    int window_size;         // Example member for window bits
    // Add additional members as needed
};


void parseReport(const std::string& filename, std::unordered_map<std::string, std::vector<ReportRow>>& report) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    std::string line;
    std::getline(file, line); // Read header line
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        ReportRow row;
        std::string temp;
        std::getline(iss, row.file_name, ',');
        std::getline(iss, temp, ',');
        row.original_size = std::stod(temp);
        std::getline(iss, temp, ',');
        row.compression_quality = std::stoi(temp);
        std::getline(iss, temp, ',');
        row.window_size = std::stoi(temp);
        std::getline(iss, temp, ',');
        row.brotli_time = std::stod(temp);
        std::getline(iss, temp, ',');
        row.compression_time = std::stod(temp);
        std::getline(iss, temp, ',');
        row.compression_size = std::stod(temp);
        // Read other columns as needed
        std::getline(iss, temp, ','); // Skip Compression Ratio
        std::getline(iss, temp, ','); // Skip CPU Usage by Process
        std::getline(iss, temp);      // Read Maximum Resident Size
        row.cpu_usage = std::stod(temp);

        report[row.file_name].push_back(row);
    }
}

void writeComparisonReport(const std::unordered_map<std::string, std::vector<ReportRow>>& report1,
                           const std::unordered_map<std::string, std::vector<ReportRow>>& report2) {
    std::ofstream outfile("_comparison_report_c_" +
                          std::to_string(report1.begin()->second[0].compression_quality) + "_" +
                          std::to_string(report2.begin()->second[0].compression_quality) + "_" +
                          "w" + "_" +
                          std::to_string(report1.begin()->second[0].window_size) + "_" +
                          std::to_string(report2.begin()->second[0].window_size) + 
                          "v110"+
                          ".csv");

    outfile << "File Name,Original File Size,Change in Compression Size (%),"
            << "Change in Compression Time (%),Change in Brotli Time (%),Change in CPU Usage (%)\n";

    for (const auto& pair1 : report1) {
        const std::string& file_name = pair1.first;
        const auto& rows1 = pair1.second;
        const auto& rows2 = report2.at(file_name);

        double sum_compression_size_diff = 0.0;
        double sum_compression_time_diff = 0.0;
        double sum_brotli_time_diff = 0.0;
        double sum_cpu_usage_diff = 0.0;

        size_t num_rows = std::min(rows1.size(), rows2.size());

        for (size_t i = 0; i < num_rows; ++i) {
            sum_compression_size_diff += ((rows2[i].compression_size - rows1[i].compression_size) * 100) / rows1[i].compression_size;
            sum_compression_time_diff += ((rows2[i].compression_time - rows1[i].compression_time) * 100) / rows1[i].compression_time;
            sum_brotli_time_diff += ((rows2[i].brotli_time - rows1[i].brotli_time) * 100) / rows1[i].brotli_time;
            sum_cpu_usage_diff += ((rows2[i].cpu_usage - rows1[i].cpu_usage) * 100) / rows1[i].cpu_usage;
        }

        double avg_compression_size_diff = sum_compression_size_diff / num_rows;
        double avg_compression_time_diff = sum_compression_time_diff / num_rows;
        double avg_brotli_time_diff = sum_brotli_time_diff / num_rows;
        double avg_cpu_usage_diff = sum_cpu_usage_diff / num_rows;

        outfile << file_name << "," << rows1[0].original_size << ","
                << avg_compression_size_diff << "," << avg_compression_time_diff << ","
                << avg_brotli_time_diff << "," << avg_cpu_usage_diff << "\n";
    }

    // Information section
    outfile << "\n\n";
    outfile << "Note:\n";
    outfile << "Percentage values represent changes in values that were compressed using compression level " << report1.begin()->second[0].compression_quality
            << " and window size " << report1.begin()->second[0].window_size << " to compression level "
            << report2.begin()->second[0].compression_quality << " and window size " << report2.begin()->second[0].window_size << ".\n";
    outfile << "A negative value indicates a percentage decrease from the old value to the new value.\n";
    outfile << "A positive value indicates a percentage increase.\n";

    outfile.close();
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <report_file1> <report_file2>\n";
        return 1;
    }

    std::string report_file1 = argv[1];
    std::string report_file2 = argv[2];

    std::unordered_map<std::string, std::vector<ReportRow>> report1, report2;

    parseReport(report_file1, report1);
    parseReport(report_file2, report2);

    writeComparisonReport(report1, report2);

    std::cout << "Comparison report generated successfully.\n";

    return 0;
}
