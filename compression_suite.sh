#!/bin/bash

# Check if the correct number of arguments is provided
if [ "$#" -ne 3 ]; then
  echo "Usage: $0 <old|new> <compression_quality> <directory>"
  exit 1
fi

# Assign input arguments to variables
mode=$1
compression_quality=$2
directory=$3

# Determine which executable to use
if [ "$mode" == "old" ]; then
  executable="./final_test_old"
elif [ "$mode" == "new" ]; then
  executable="./final_test_new"
else
  echo "Invalid mode. Use 'old' or 'new'."
  exit 1
fi

# Check if the directory exists
if [ ! -d "$directory" ]; then
  echo "Directory $directory does not exist."
  exit 1
fi

# Get the base name of the directory
base_directory=$(basename "$directory")
parent_directory=$(dirname "$directory")
suite_directory="${parent_directory}/${base_directory}_suite"

# Create the suite directory if it doesn't exist
mkdir -p "$suite_directory"

# Function to perform compression and logging
compress_and_log() {
  local file=$1
  local quality=$2
  local window_size=$3
  local output_dir=$4
  local report_file="${output_dir}/_report_c${quality}_w${window_size}.csv"

  # Create the compressed directory if it doesn't exist
  mkdir -p "$output_dir"

  # Write the header to the CSV file if it doesn't exist
  if [ ! -f "$report_file" ]; then
    echo "Original File Name,File Size(B),Compression Level,Window Bits,Time Taken by Brotli(s),Time Taken in Compression(s),Compressed File Size(B),Compression Ratio,CPU Usage by Compression Process(%),Maximum Resident Size(B)" > "$report_file"
  fi

  for i in {1..3}; do
    output=$($executable -f "$file" -c "$quality" -w "$window_size" -m compress)
    
    original_file_name=$(basename "$file")
    file_size=$(echo "$output" | grep "Original File Size:" | cut -d ' ' -f 4)
    time_taken_by_brotli=$(echo "$output" | grep "Time taken by Brotli for compression:" | awk '{print $7}' | tr -d 's')
    time_taken_in_compression=$(echo "$output" | grep "Time taken by Full Compression:" | awk '{print $6}' | tr -d 's')
    compressed_file_size=$(echo "$output" | grep "Compressed File Size:" | cut -d ' ' -f 4)
    compression_ratio=$(echo "$output" | grep "Compression Ratio:" | cut -d ' ' -f 3)
    cpu_usage=$(echo "$output" | grep "CPU usage by process:" | cut -d ' ' -f 5 | tr -d '%')
    max_resident_size=$(echo "$output" | grep "Maximum resident set size:" | cut -d ' ' -f 5)
    
    echo "$original_file_name,$file_size,$quality,$window_size,$time_taken_by_brotli,$time_taken_in_compression,$compressed_file_size,$compression_ratio,$cpu_usage,$max_resident_size" >> "$report_file"

    # Move the compressed file to the new directory
    compressed_file="${file}.br"
    if [ -f "$compressed_file" ]; then
      mv "$compressed_file" "$output_dir/"
    fi
  done
}

# Loop through all files in the directory
for file in "$directory"/*; do
  if [ -f "$file" ]; then
    for window_size in {16..22}; do
      compressed_directory="${suite_directory}/${base_directory}_compressed_c${compression_quality}_w${window_size}"
      compress_and_log "$file" "$compression_quality" "$window_size" "$compressed_directory"
    done
  fi
done

echo "Compression complete. Reports are available in respective directories within $suite_directory."
