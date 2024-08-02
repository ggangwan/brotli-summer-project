#include <iostream>
#include <thread>
#include <chrono>
#include <sys/types.h>
#include <libproc.h>
#include <sys/resource.h>
#include <unistd.h>

long getRSS(pid_t pid) {
    struct proc_taskinfo pti;
    int ret = proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &pti, sizeof(pti));
    if (ret == sizeof(pti)) {
        return pti.pti_resident_size; // Return raw bytes
    } else {
        return -1; // Error case
    }
}

void printMaxRSS() {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        std::cout << "Maximum resident set size (self): " << usage.ru_maxrss << " KB" << std::endl;
    } else {
        std::cerr << "Error: Unable to retrieve resource usage." << std::endl;
    }
}

void monitorMemoryUsage(pid_t pid) {
    long lastRSS = -1;
    while (true) {
        long currentRSS = getRSS(pid);
        if (currentRSS != lastRSS) {
            lastRSS = currentRSS;
            if (currentRSS == -1) {
                std::cerr << "Error: Unable to retrieve memory usage for PID " << pid << ". The process might have terminated." << std::endl;
                break;
            } else {
                std::cout << "Current RSS of PID " << pid << ": " << currentRSS << " bytes" << std::endl;
            }
        }
        // Continuous monitoring without sleep
    }
}

void logMemoryUsage() {
    while (true) {
        printMaxRSS();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main() {
    pid_t pid;
    std::cout << "Enter PID to monitor: ";
    std::cin >> pid;

    std::thread monitor(monitorMemoryUsage, pid);
    monitor.detach();

    std::cout << "Monitoring memory usage of PID " << pid << "..." << std::endl;

    // Log max RSS of the current process periodically
    std::thread logger(logMemoryUsage);
    logger.detach();

    // Run the main thread for a while to keep the program running
    std::this_thread::sleep_for(std::chrono::minutes(5));

    return 0;
}
