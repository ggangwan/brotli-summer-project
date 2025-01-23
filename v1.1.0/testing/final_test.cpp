#include <iostream>
#include <fstream>
#include <brotli/encode.h>
#include <brotli/decode.h>
#include <brotli/constants.h>
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

static int64_t FileSize(const char* path) {
  FILE* f = fopen(path, "rb");
  int64_t retval;
  if (f == NULL) {
    return -1;
  }
  if (fseek(f, 0L, SEEK_END) != 0) {
    fclose(f);
    return -1;
  }
  retval = ftell(f);
  if (fclose(f) != 0) {
    return -1;
  }
  return retval;
}

static const BrotliEncoderPreparedDictionary* ReadDictionary(const char* dictionary_path, bool compress) {
  static const int kMaxDictionarySize =
      BROTLI_MAX_DISTANCE - BROTLI_MAX_BACKWARD_LIMIT(24);
  FILE* f;
  int64_t file_size_64;
  uint8_t* buffer;
  size_t bytes_read;

  f = fopen(dictionary_path, "rb");
  if (f == NULL) {
    fprintf(stderr, "failed to open dictionary file [%s]: %s\n",
            dictionary_path, strerror(errno));
    return nullptr;
  }

  file_size_64 = FileSize(dictionary_path);
  if (file_size_64 == -1) {
    fprintf(stderr, "could not get size of dictionary file [%s]",
            dictionary_path);
    fclose(f);
    return nullptr;
  }

  if (file_size_64 > kMaxDictionarySize) {
    fprintf(stderr, "dictionary [%s] is larger than maximum allowed: %d\n",
            dictionary_path, kMaxDictionarySize);
    fclose(f);
    return nullptr;
  }
  size_t dictionary_size = (size_t)file_size_64;

  buffer = (uint8_t*)malloc(dictionary_size);
  if (!buffer) {
    fprintf(stderr, "could not read dictionary: out of memory\n");
    fclose(f);
    return nullptr;
  }
  bytes_read = fread(buffer, sizeof(uint8_t), dictionary_size, f);
  if (bytes_read != dictionary_size) {
    free(buffer);
    fprintf(stderr, "failed to read dictionary [%s]: %s\n",
            dictionary_path, strerror(errno));
    fclose(f);
    return nullptr;
  }
  fclose(f);
  uint8_t* dictionary = buffer;
  const BrotliEncoderPreparedDictionary* prepared_dictionary;
  if (compress) {
    prepared_dictionary = BrotliEncoderPrepareDictionary(
        BROTLI_SHARED_DICTIONARY_RAW, dictionary_size,
        dictionary, BROTLI_MAX_QUALITY, NULL, NULL, NULL);
    if (prepared_dictionary == NULL) {
      fprintf(stderr, "failed to prepare dictionary [%s]\n",
              dictionary_path);
      return nullptr;
    }
  }
  return prepared_dictionary;
}

bool CompressData(const std::string& input_filename, const std::string& output_filename, int quality=6, int lgwin=16, double* timetakenbyBECP = nullptr, const char* dictionary_path = nullptr) {
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


    //Call to ReadDictionary -> returns const BrotliEncoderPreparedDictionary* -> if NULL exit
    const BrotliEncoderPreparedDictionary* prepared_dictionary = nullptr;
    if(dictionary_path){
        prepared_dictionary = ReadDictionary(dictionary_path, true);
        if(!prepared_dictionary){
            std::cerr << "Failed to prepare dictionary." << std::endl;
            return false;
        }
    }

    BrotliEncoderState* encoder_state = BrotliEncoderCreateInstance(nullptr, nullptr, nullptr);
    if (!encoder_state) {
        std::cerr << "Failed to create Brotli encoder state." << std::endl;
        return false;
    }

    BrotliEncoderSetParameter(encoder_state, BROTLI_PARAM_QUALITY, quality);
    BrotliEncoderSetParameter(encoder_state, BROTLI_PARAM_LGWIN, lgwin);


    //Call to BrotliEncoderAttachPreparedDictionary -> returns BROTLI_BOOL -> if false exit
    if(prepared_dictionary){
        if(BrotliEncoderAttachPreparedDictionary(encoder_state, prepared_dictionary) == BROTLI_FALSE){
            std::cerr << "Failed to attach dictionary." << std::endl;
            return false;
        }
    }

    uint8_t input_buffer[BUFFER_SIZE];
    uint8_t output_buffer[BUFFER_SIZE];
    memset(input_buffer, 0, BUFFER_SIZE);
    memset(output_buffer, 0, BUFFER_SIZE);
    bool is_eof = false;

    double totalTimeBrotliEncoderComrpessStream = 0.0;

    while (!is_eof) {
        input_file.read(reinterpret_cast<char*>(input_buffer), BUFFER_SIZE);
        std::streamsize bytes_read = input_file.gcount();
        is_eof = input_file.eof();

        if (bytes_read > 0) {
            const uint8_t* next_in = input_buffer;
            size_t available_in = static_cast<size_t>(bytes_read);

            do {
                uint8_t* next_out = output_buffer;
                size_t available_out = BUFFER_SIZE;

                struct timeval startTime, endTime;

                // Measure time taken by BrotliEncoderCompressStream
                gettimeofday(&startTime, nullptr);
                if (!BrotliEncoderCompressStream(encoder_state,
                    is_eof ? BROTLI_OPERATION_FINISH : BROTLI_OPERATION_PROCESS,
                    &available_in, &next_in, &available_out, &next_out, nullptr)) {
                    std::cerr << "Brotli compression failed." << std::endl;
                    BrotliEncoderDestroyInstance(encoder_state);
                    return false;
                }
                gettimeofday(&endTime, nullptr);
                totalTimeBrotliEncoderComrpessStream += (endTime.tv_sec - startTime.tv_sec) + (endTime.tv_usec - startTime.tv_usec) / 1e6;

                size_t output_size = BUFFER_SIZE - available_out;
                output_file.write(reinterpret_cast<char*>(output_buffer), output_size);
            } while (available_in > 0 || BrotliEncoderHasMoreOutput(encoder_state));
        }
    }

    BrotliEncoderDestroyInstance(encoder_state);

    // Update pointer with the total time taken
    if (timetakenbyBECP) {
        *timetakenbyBECP = totalTimeBrotliEncoderComrpessStream;
    }

    return true;
}

bool DecompressData(const std::string& input_file, const std::string& output_file, const char* dictionary_path = nullptr) {
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

    // Print the filenames based on the mode
    if (mode == "compress") {
        std::cout << "Input file: " << file_path << "\n -> Compressed file: " << compressed_file << std::endl;
    } else if (mode == "decompress") {
        std::cout << "Input file: " << compressed_file << "\n -> Decompressed file: " << decompressed_file << std::endl;
    } else if (mode == "both") {
        std::cout << "Input file: " << file_path << "\n -> Compressed file: " << compressed_file << "\n -> Decompressed file: " << decompressed_file << std::endl;
    }

    timeval preTime, midTime, postTime;
    CpuUsage preProcUsage, midProcUsage, postProcUsage;
    SystemCpuUsage preSysUsage, midSysUsage, postSysUsage;
    double elapsedTimeCompression = 0.0, 
           elapsedTimeBrotliEncoderCompressStream = 0.0,
           elapsedTimeDecompression = 0.0;

    if (mode == "compress" || mode == "both") {
        gettimeofday(&preTime, nullptr);
        preProcUsage = getCpuUsage();
        preSysUsage = getSystemCpuUsage();

        if (CompressData(file_path, compressed_file, compression_quality, window_bits, &elapsedTimeBrotliEncoderCompressStream)) {
            std::cout << "Compression successful\n";
        } else {
            std::cerr << "Compression failed\n";
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

    if (mode == "compress" || mode == "both") {
        std::cout << "Time taken by Brotli for compression: " << elapsedTimeBrotliEncoderCompressStream << "s\n";
        std::cout << "Time taken by Full Compression: " << elapsedTimeCompression << "s\n";
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
            std::cout << "Original File Size: " << original_size << std::endl;
            std::cout << "Compressed File Size: " << compressed_size << std::endl;
            std::cout << "Compression Ratio: " << compression_ratio << std::endl;
        } else {
            std::cerr << "Error calculating file sizes.\n";
        }
    }

    std::cout<<std::endl;
    std::cout<<std::endl;
    return 0;
}