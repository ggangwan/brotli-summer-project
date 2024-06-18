#include <iostream>
#include <fstream>
#include "encode.h"
#include "decode.h"
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <mach/mach.h>
#include <sys/stat.h>
#include <cstring>
#include <libgen.h>

const size_t BUFFER_SIZE = 16384; // 16kB

struct CpuUsage {
    timeval utime;
    timeval stime;
};

struct SystemCpuUsage {
    uint64_t user;
    uint64_t nice;
    uint64_t system;
    uint64_t idle;
};

CpuUsage getCpuUsage() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    CpuUsage cpuUsage = {usage.ru_utime, usage.ru_stime};
    return cpuUsage;
}

double calculateCpuUsage(const CpuUsage& start, const CpuUsage& end, double elapsedTime) {
    //std::cout << "ProcCPU\n"<<end.utime.tv_sec<<"\t" << start.utime.tv_sec<<"\t" << end.utime.tv_usec<<"\t" << start.utime.tv_usec << std::endl;
    //std::cout << end.stime.tv_sec<<"\t" << start.stime.tv_sec<<"\t" << end.stime.tv_usec<<"\t" << start.stime.tv_usec << std::endl;
    double userTime = (end.utime.tv_sec - start.utime.tv_sec) + (end.utime.tv_usec - start.utime.tv_usec) / 1e6;
    double sysTime = (end.stime.tv_sec - start.stime.tv_sec) + (end.stime.tv_usec - start.stime.tv_usec) / 1e6;
    //std::cout << userTime << "\t" << sysTime << std::endl;
    double totalTime = userTime + sysTime;
    return (totalTime / elapsedTime) * 100.0;
}

SystemCpuUsage getSystemCpuUsage() {
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    host_cpu_load_info_data_t cpuInfo;
    if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, (host_info_t)&cpuInfo, &count) != KERN_SUCCESS) {
        std::cerr << "Failed to get CPU load info." << std::endl;
        exit(1);
    }
    SystemCpuUsage usage;
    usage.user = cpuInfo.cpu_ticks[CPU_STATE_USER];
    usage.nice = cpuInfo.cpu_ticks[CPU_STATE_NICE];
    usage.system = cpuInfo.cpu_ticks[CPU_STATE_SYSTEM];
    usage.idle = cpuInfo.cpu_ticks[CPU_STATE_IDLE];
    return usage;
}

double calculateOverallCpuUsage(const SystemCpuUsage& start, const SystemCpuUsage& end) {
    //std::cout << "OverallCPU\n"<<start.user << "\t"<< start.nice << "\t"<< start.system << "\t"<< start.idle << std::endl;
    //std::cout << end.user << "\t"<< end.nice << "\t"<< end.system << "\t"<< end.idle << std::endl;
    uint64_t totalStart = start.user + start.nice + start.system + start.idle;
    uint64_t totalEnd = end.user + end.nice + end.system + end.idle;

    uint64_t totalUser = end.user - start.user;
    uint64_t totalSystem = end.system - start.system;

    uint64_t totalDelta = totalEnd - totalStart;

    // Prevent division by zero
    if (totalDelta == 0) {
        return 0.0;
    }
    double cpuUsage = 100.0 * (totalUser + totalSystem) / totalDelta;
    return cpuUsage;
}

void printMaxRSS() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    std::cout << "Maximum resident set size: " << usage.ru_maxrss << " kilobytes\n";
}

bool CompressData(const std::string& input_file, const std::string& output_file, int quality = 6, int lgwin = 16, double* timeTakenByCITRB = nullptr, double* timeTakenByWBD = nullptr) {
    std::ifstream in(input_file, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "Error opening input file: " << input_file << std::endl;
        return false;
    }

    std::ofstream out(output_file, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Error opening output file: " << output_file << std::endl;
        return false;
    }

    brotli::BrotliParams params;
    params.quality = quality; 
    params.lgwin = lgwin;   
    brotli::BrotliCompressor compressor(params);

    uint8_t input_buffer[BUFFER_SIZE];
    memset(input_buffer, 0, BUFFER_SIZE);
    size_t output_size = 0;
    uint8_t* output_ptr = nullptr;

    // Variables to hold the time taken by each function
    double totalTimeCopyInputToRingBuffer = 0.0;
    double totalTimeWriteBrotliData = 0.0;

    while (!in.eof()) {
        in.read(reinterpret_cast<char*>(input_buffer), BUFFER_SIZE);
        if (in.bad()) {
            std::cerr << "Error reading input file: " << input_file << std::endl;
            return false;
        }
        size_t input_size = in.gcount();
        if (input_size == 0) {
            break;
        }

        struct timeval startTime, endTime;
        
        // Measure time taken by CopyInputToRingBuffer
        gettimeofday(&startTime, nullptr);
        compressor.CopyInputToRingBuffer(input_size, input_buffer);
        gettimeofday(&endTime, nullptr);
        totalTimeCopyInputToRingBuffer += (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec) / 1e6;

        // Measure time taken by WriteBrotliData
        gettimeofday(&startTime, nullptr);
        if (!compressor.WriteBrotliData(in.eof(), true, &output_size, &output_ptr) || output_size > BUFFER_SIZE) {
            std::cerr << "Error compressing data" << std::endl;
            return false;
        }
        gettimeofday(&endTime, nullptr);
        totalTimeWriteBrotliData += (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec) / 1e6;

        if (output_size > 0) {
            out.write(reinterpret_cast<char*>(output_ptr), output_size);
            if (out.bad()) {
                std::cerr << "Error writing output file: " << output_file << std::endl;
                return false;
            }
        }

        if (output_size == 0) {
            break;
        }
    }

    // Update the pointers with the total time taken
    if (timeTakenByCITRB) {
        *timeTakenByCITRB = totalTimeCopyInputToRingBuffer;
    }
    if (timeTakenByWBD) {
        *timeTakenByWBD = totalTimeWriteBrotliData;
    }

    return true;
}

bool DecompressData(const std::string& input_file, const std::string& output_file) {
    std::ifstream in(input_file, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "Error opening input file\n";
        return false;
    }

    std::ofstream out(output_file, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Error opening output file\n";
        return false;
    }

    // Define buffer
    uint8_t input_buffer[BUFFER_SIZE];
    uint8_t output_buffer[BUFFER_SIZE];
    memset(input_buffer, 0, BUFFER_SIZE);
    memset(output_buffer, 0, BUFFER_SIZE);

    // Initialize Brotli state
    BrotliState* state = BrotliCreateState(nullptr, nullptr, nullptr);
    if (state == nullptr) {
        std::cerr << "Error creating Brotli state\n";
        return false;
    }

    size_t total_out = 0;
    size_t available_out;

    while (true) {
        // Read input data into buffer
        in.read(reinterpret_cast<char*>(input_buffer), BUFFER_SIZE);
        size_t available_in = in.gcount();
        if (available_in == 0 && in.eof()) {
            break; // End of input file
        }

        const uint8_t* next_in = input_buffer;
        available_out = BUFFER_SIZE;
        uint8_t* next_out = output_buffer;

        while (true) {
            // Decompress data
            BrotliResult result = BrotliDecompressStream(&available_in, &next_in, &available_out, &next_out, &total_out, state);
            switch (result) {
                case BROTLI_RESULT_SUCCESS:
                    // Write the remaining decompressed data to the output file
                    out.write(reinterpret_cast<char*>(output_buffer), BUFFER_SIZE - available_out);
                    if (!out) {
                        std::cerr << "Error writing to output file\n";
                        BrotliDestroyState(state);
                        return false;
                    }
                    BrotliDestroyState(state);
                    return true;

                case BROTLI_RESULT_NEEDS_MORE_INPUT:
                    // Write the partially decompressed data to the output file
                    out.write(reinterpret_cast<char*>(output_buffer), BUFFER_SIZE - available_out);
                    if (!out) {
                        std::cerr << "Error writing to output file\n";
                        BrotliDestroyState(state);
                        return false;
                    }
                    // Break to read more input
                    break;

                case BROTLI_RESULT_NEEDS_MORE_OUTPUT:
                    // Write the decompressed data to the output file
                    out.write(reinterpret_cast<char*>(output_buffer), BUFFER_SIZE - available_out);
                    if (!out) {
                        std::cerr << "Error writing to output file\n";
                        BrotliDestroyState(state);
                        return false;
                    }
                    // Reset the output buffer
                    available_out = BUFFER_SIZE;
                    next_out = output_buffer;
                    break;

                default:
                    std::cerr << "Error decompressing data\n";
                    BrotliDestroyState(state);
                    return false;
            }

            if (result == BROTLI_RESULT_NEEDS_MORE_INPUT) {
                break; // Break to read more input
            }
        }
    }

    // Write any remaining decompressed data to the output file
    size_t output_size = BUFFER_SIZE - available_out;
    if (output_size > 0) {
        out.write(reinterpret_cast<char*>(output_buffer), output_size);
        if (!out) {
            std::cerr << "Error writing to output file\n";
            BrotliDestroyState(state);
            return false;
        }
    }

    // Clean up
    BrotliDestroyState(state);
    return true;
}

size_t GetFileSize(const std::string& filename) {
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}
void PrintUsage() {
    std::cout << "Usage: program_name -f <file_path> -c <compression_quality> -w <window_bits> -m <mode>\n"
              << "  -f <file_path>              : Path to the input file\n"
              << "  -c <compression_quality>    : Compression quality (1 to 11)\n"
              << "  -w <window_bits>            : Number of window bits (10 to 24)\n"
              << "  -m <mode>                   : Mode ('compress', 'decompress', 'both')\n";
}

int main(int argc, char* argv[]) {
    std::string file_path;
    int compression_quality = 6;
    int window_bits = 16;
    std::string mode = "both";

    int opt;
    while ((opt = getopt(argc, argv, "f:c:w:m:")) != -1) {
        switch (opt) {
            case 'f':
                file_path = optarg;
                break;
            case 'c':
                compression_quality = std::stoi(optarg);
                break;
            case 'w':
                window_bits = std::stoi(optarg);
                break;
            case 'm':
                mode = optarg;
                break;
            default:
                PrintUsage();
                return 1;
        }
    }

    if (file_path.empty() || (mode != "compress" && mode != "decompress" && mode != "both")) {
        PrintUsage();
        return 1;
    }

    // Extract the directory and base name of the file for constructing output file paths
    char* file_path_cstr = new char[file_path.length() + 1];
    std::strcpy(file_path_cstr, file_path.c_str());
    std::string dir_name = dirname(file_path_cstr);
    std::string base_name = basename(file_path_cstr);
    delete[] file_path_cstr;

    std::string compressed_file;
    std::string decompressed_file;

    if (mode == "compress" || mode == "both") {
        compressed_file = dir_name + "/" + base_name + ".br";
    }

    if (mode == "decompress" || mode == "both") {
        if (mode == "decompress") {
            compressed_file = file_path;
            if (base_name.size() >= 3 && base_name.substr(base_name.size() - 3) == ".br") {
                decompressed_file = dir_name + "/d-" + base_name.substr(0, base_name.size() - 3);
            } else {
                std::cerr << "Invalid file extension for decompression mode.\n";
                return 1;
            }
        } else {
            decompressed_file = dir_name + "/d-" + base_name;
        }
    }

    // Display the path where the operation starts
    std::cout << "\nOperation starts in directory: " << dir_name << std::endl;

    // Print the filenames based on the mode
    if (mode == "compress") {
        std::cout << "Input file: " << file_path << " -> Compressed file: " << compressed_file << std::endl;
    } else if (mode == "decompress") {
        std::cout << "Input file: " << compressed_file << " -> Decompressed file: " << decompressed_file << std::endl;
    } else if (mode == "both") {
        std::cout << "Input file: " << file_path << " -> Compressed file: " << compressed_file << " -> Decompressed file: " << decompressed_file << std::endl;
    }

    timeval preTime, midTime, postTime;
    CpuUsage preProcUsage, midProcUsage, postProcUsage;
    SystemCpuUsage preSysUsage, midSysUsage, postSysUsage;
    double elapsedTimeCompression = 0.0, 
           elapsedTimeCopyInputToRingBuffer = 0.0,
           elapsedTimeWriteBrotliData = 0.0,
           elapsedTimeDecompression = 0.0;

    if (mode == "compress" || mode == "both") {
        gettimeofday(&preTime, nullptr);
        preProcUsage = getCpuUsage();
        preSysUsage = getSystemCpuUsage();

        if (CompressData(file_path, compressed_file, compression_quality, window_bits, &elapsedTimeCopyInputToRingBuffer, &elapsedTimeWriteBrotliData)) {
            std::cout << "\nCompression successful\n";
        } else {
            std::cerr << "\nCompression failed\n";
            return 1;
        }

        gettimeofday(&midTime, nullptr);
        midProcUsage = getCpuUsage();
        midSysUsage = getSystemCpuUsage();

        elapsedTimeCompression = (midTime.tv_sec - preTime.tv_sec) + (midTime.tv_usec - preTime.tv_usec) / 1e6;
    }

    if (mode == "decompress" || mode == "both") {
        if (mode == "decompress") {
            gettimeofday(&preTime, nullptr);
            preProcUsage = getCpuUsage();
            preSysUsage = getSystemCpuUsage();
        }

        if (DecompressData(compressed_file, decompressed_file)) {
            std::cout << "Decompression successful\n";
        } else {
            std::cerr << "Decompression failed\n";
            return 1;
        }

        gettimeofday(&postTime, nullptr);
        postProcUsage = getCpuUsage();
        postSysUsage = getSystemCpuUsage();

        if (mode == "both") {
            elapsedTimeDecompression = (postTime.tv_sec - midTime.tv_sec) + (postTime.tv_usec - midTime.tv_usec) / 1e6;
        } else {
            elapsedTimeDecompression = (postTime.tv_sec - preTime.tv_sec) + (postTime.tv_usec - preTime.tv_usec) / 1e6;
        }
    }

    double elapsedTimeTotal = elapsedTimeCompression + elapsedTimeDecompression;

    std::cout<<std::endl;

    if (mode == "compress" || mode == "both") {
        std::cout << "Time taken by CopyInputToRingBuffer: " << elapsedTimeCopyInputToRingBuffer << "s\n";
        std::cout << "Time taken by WriteBrotliData: " << elapsedTimeWriteBrotliData << "s\n";
        std::cout << "Time taken by Compression: " << elapsedTimeCompression << "s\n";
    }

    if (mode == "decompress" || mode == "both") {
        std::cout << "Time taken by Decompression: " << elapsedTimeDecompression << "s\n";
    }

    if (mode == "both") {
        std::cout << "Total time taken: " << elapsedTimeTotal << "s\n\n";
    }

    std::cout<<std::endl;

    if (mode == "compress") {
        double sysCpuUsage = calculateOverallCpuUsage(preSysUsage, midSysUsage);
        double procCpuUsage = calculateCpuUsage(preProcUsage, midProcUsage, elapsedTimeCompression);
        std::cout << "CPU usage by process: " << procCpuUsage << "%\n";
        std::cout << "Overall CPU usage: " << sysCpuUsage << "%\n";
    } else {
        double sysCpuUsage = calculateOverallCpuUsage(preSysUsage, postSysUsage);
        double procCpuUsage = calculateCpuUsage(preProcUsage, postProcUsage, elapsedTimeTotal);
        std::cout << "CPU usage by process: " << procCpuUsage << "%\n";
        std::cout << "Overall CPU usage: " << sysCpuUsage << "%\n";
    }

    std::cout<<std::endl;
    printMaxRSS();
    std::cout<<std::endl;

    if (mode == "compress" || mode == "both") {
        size_t original_size = GetFileSize(file_path);
        size_t compressed_size = GetFileSize(compressed_file);

        if (original_size != -1 && compressed_size != -1) {
            double compression_ratio = static_cast<double>(original_size) / compressed_size;
            std::cout << "Compression Ratio: " << compression_ratio << std::endl;
        } else {
            std::cerr << "Error calculating file sizes.\n";
        }
    }

    return 0;
}