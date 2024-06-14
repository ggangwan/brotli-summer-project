#include <iostream>
#include <fstream>
#include <brotli/encode.h>
#include <brotli/decode.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <mach/mach.h>
#include <sys/stat.h>
#include <cstring>

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

bool CompressData(const std::string& input_filename, const std::string& output_filename, int quality=6, int lgwin=16) {
    std::ifstream input_file(input_filename, std::ios::binary);
    if (!input_file) {
        std::cerr << "Failed to open input file: " << input_filename << std::endl;
        return false;
    }

    std::ofstream output_file(output_filename, std::ios::binary);
    if (!output_file) {
        std::cerr << "Failed to open output file: " << output_filename << std::endl;
        return false;
    }

    BrotliEncoderState* encoder_state = BrotliEncoderCreateInstance(nullptr, nullptr, nullptr);
    if (!encoder_state) {
        std::cerr << "Failed to create Brotli encoder state." << std::endl;
        return false;
    }

    BrotliEncoderSetParameter(encoder_state, BROTLI_PARAM_QUALITY, quality);
    BrotliEncoderSetParameter(encoder_state, BROTLI_PARAM_LGWIN, lgwin);

    uint8_t input_buffer[BUFFER_SIZE];
    uint8_t output_buffer[BUFFER_SIZE];
    memset(input_buffer, 0, BUFFER_SIZE);
    memset(output_buffer,0, BUFFER_SIZE);
    bool is_eof = false;

    while (!is_eof) {
        input_file.read(reinterpret_cast<char*>(input_buffer), BUFFER_SIZE);
        std::streamsize bytes_read = input_file.gcount();
        is_eof = input_file.eof();

        if (bytes_read > 0) {
            const uint8_t* next_in = input_buffer;
            size_t available_in = static_cast<size_t>(bytes_read);

            while (available_in > 0) {
                uint8_t* next_out = output_buffer;
                size_t available_out = BUFFER_SIZE;

                if (!BrotliEncoderCompressStream(encoder_state,
                    is_eof ? BROTLI_OPERATION_FINISH : BROTLI_OPERATION_PROCESS,
                    &available_in, &next_in, &available_out, &next_out, nullptr)) {
                    std::cerr << "Brotli compression failed." << std::endl;
                    BrotliEncoderDestroyInstance(encoder_state);
                    return false;
                }

                size_t output_size = BUFFER_SIZE - available_out;
                output_file.write(reinterpret_cast<char*>(output_buffer), output_size);
            }
        }
    }

    BrotliEncoderDestroyInstance(encoder_state);
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
    memset(output_buffer,0, BUFFER_SIZE);

    // Initialize Brotli decoder state
    BrotliDecoderState* state = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
    if (state == nullptr) {
        std::cerr << "Error creating Brotli decoder state\n";
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
            BrotliDecoderResult result = BrotliDecoderDecompressStream(state, &available_in, &next_in, &available_out, &next_out, &total_out);
            switch (result) {
                case BROTLI_DECODER_RESULT_SUCCESS:
                    // Write the remaining decompressed data to the output file
                    out.write(reinterpret_cast<char*>(output_buffer), BUFFER_SIZE - available_out);
                    if (!out) {
                        std::cerr << "Error writing to output file\n";
                        BrotliDecoderDestroyInstance(state);
                        return false;
                    }
                    BrotliDecoderDestroyInstance(state);
                    return true;

                case BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT:
                    // Write the partially decompressed data to the output file
                    out.write(reinterpret_cast<char*>(output_buffer), BUFFER_SIZE - available_out);
                    if (!out) {
                        std::cerr << "Error writing to output file\n";
                        BrotliDecoderDestroyInstance(state);
                        return false;
                    }
                    // Break to read more input
                    break;

                case BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT:
                    // Write the decompressed data to the output file
                    out.write(reinterpret_cast<char*>(output_buffer), BUFFER_SIZE - available_out);
                    if (!out) {
                        std::cerr << "Error writing to output file\n";
                        BrotliDecoderDestroyInstance(state);
                        return false;
                    }
                    // Reset the output buffer
                    available_out = BUFFER_SIZE;
                    next_out = output_buffer;
                    break;

                case BROTLI_DECODER_RESULT_ERROR:
                    std::cerr << "Error decompressing data\n";
                    BrotliDecoderDestroyInstance(state);
                    return false;

                default:
                    std::cerr << "Unexpected result from Brotli decoder\n";
                    BrotliDecoderDestroyInstance(state);
                    return false;
            }

            if (result == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT) {
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
            BrotliDecoderDestroyInstance(state);
            return false;
        }
    }

    // Clean up
    BrotliDecoderDestroyInstance(state);
    return true;
}

size_t GetFileSize(const std::string& filename) {
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

int main() {
    std::string input_file = "sample.txt";
    std::string compressed_file = "compressed.br";
    std::string decompressed_file = "decompressed.txt";

    std::cout<<input_file<<" -> " << compressed_file << " -> " << decompressed_file << std::endl;

    timeval preTime, midTime, postTime;

    // Record time and resources pre-compression
    gettimeofday(&preTime, nullptr);
    CpuUsage preProcUsage = getCpuUsage();
    SystemCpuUsage preSysUsage = getSystemCpuUsage();

    // Compress data
    if (CompressData(input_file, compressed_file, 6, 16)) {
        std::cout << "Compression successful\n";
    } else {
        std::cerr << "Compression failed\n";
        return 1;
    }

    // Record time after compression, before decompression
    gettimeofday(&midTime, nullptr);

    // Decompress data
    if (DecompressData(compressed_file, decompressed_file)) {
        std::cout << "Decompression successful\n";
    } else {
        std::cerr << "Decompression failed\n";
        return 1;
    }

    // Record time and resources post-decompression
    gettimeofday(&postTime, nullptr);
    CpuUsage postProcUsage = getCpuUsage();
    SystemCpuUsage postSysUsage = getSystemCpuUsage();

    // Performance metrics
    double elapsedTimeCompression = (midTime.tv_sec - preTime.tv_sec) 
                       + (midTime.tv_usec - preTime.tv_usec) / 1e6;
    double elapsedTimeDecompression = (postTime.tv_sec - midTime.tv_sec) 
                       + (postTime.tv_usec - midTime.tv_usec) / 1e6;
    double elapsedTimeTotal = elapsedTimeCompression + elapsedTimeDecompression;

    std::cout << "Time taken by compression: " << elapsedTimeCompression << "s"<<std::endl
              << "Time taken by decompression: " << elapsedTimeDecompression << "s"<< std::endl
              << "Total time taken: " << elapsedTimeTotal << "s"<< std::endl;

    double sysCpuUsage = calculateOverallCpuUsage(preSysUsage, postSysUsage);
    double procCpuUsage = calculateCpuUsage(preProcUsage, postProcUsage, elapsedTimeTotal);

    std::cout << "CPU usage by process: " << procCpuUsage << "%" << std::endl;
    std::cout << "Overall CPU usage: " << sysCpuUsage << "%" << std::endl;

    printMaxRSS();

    // Calculate and display compression ratio
    size_t original_size = GetFileSize(input_file);
    size_t compressed_size = GetFileSize(compressed_file);
    
    if (original_size != -1 && compressed_size != -1) {
        double compression_ratio = static_cast<double>(original_size) / compressed_size;
        std::cout << "Compression Ratio: " << compression_ratio << std::endl;
    } else {
        std::cerr << "Error calculating file sizes." << std::endl;
    }

    return 0;
}
