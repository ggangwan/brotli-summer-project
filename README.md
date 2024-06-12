# Brotli Compression and Decompression Scripts

This repository contains scripts for Brotli compression and decompression for two different versions: `v0.4.0` and `v1.1.0`. The scripts are located in the `final_test.cpp` file within the respective `testing` directories.

## Directory Structure

```
brotli-git-setup/
├── v0.4.0/
│   ├── dec/
│   ├── enc/
│   └── testing/
│       ├── final_test.cpp
│       └── Makefile
└── v1.1.0/
    ├── include/
    ├── lib/
    └── testing/
        ├── final_test.cpp
        └── Makefile
```

## Building the Project

### Version 0.4.0

To build the compression and decompression script for version 0.4.0:

1. Navigate to the `v0.4.0/testing` directory:
   ```sh
   cd v0.4.0/testing
   ```

2. Run `make` to compile and create the `final_test` executable:
   ```sh
   make
   ```

3. You should see a message indicating that the build is complete:
   ```
   Build complete. Run ./final_test to execute.
   ```

4. Run the executable:
   ```sh
   ./final_test
   ```

### Version 1.1.0

To build the compression and decompression script for version 1.1.0:

1. Navigate to the `v1.1.0/testing` directory:
   ```sh
   cd v1.1.0/testing
   ```

2. Run `make` to compile and create the `final_test` executable:
   ```sh
   make
   ```

3. You should see a message indicating that the build is complete:
   ```
   Build complete. Run ./final_test to execute.
   ```

4. Run the executable:
   ```sh
   ./final_test
   ```

## Usage

The `final_test` executable in both versions contains the scripts for Brotli compression and decompression. After building the project as described above, you can run the executable to perform the desired operations.
