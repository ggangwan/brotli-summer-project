#!/bin/bash

# Default values for compression quality and window size
compression_quality=6
window_size=16

# Parse command line arguments for compression quality and window size
while getopts "c:w:" opt; do
  case $opt in
    c) compression_quality=$OPTARG ;;
    w) window_size=$OPTARG ;;
    \?) echo "Invalid option -$OPTARG" >&2; exit 1 ;;
  esac
done

# Shift the arguments to get the directory path
shift $((OPTIND -1))
directory=$1

if [ -z "$directory" ]; then
  echo "Usage: $0 [-c compression_quality] [-w window_size] <directory>"
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
compressed_directory="${parent_directory}/${base_directory}_compressed_c${compression_quality}_w${window_size}_v110"

# Create the compressed directory if it doesn't exist
mkdir -p "$compressed_directory"

# File to store the report
report_file="${compressed_directory}/_report_c${compression_quality}_w${window_size}_v110.csv"

# Write the header to the CSV file
echo "Original File Name,File Size(B),Compression Level,Window Bits,Time Taken by Brotli(s),Time Taken in Compression(s),Compressed File Size(B),Compression Ratio,CPU Usage by Compression Process(%),Maximum Resident Size(B)" > "$report_file"

# Loop through all files in the directory
for file in "$directory"/*; do
  if [ -f "$file" ]; then
    for i in {1..3}; do
      output=$(./final_test -f "$file" -c "$compression_quality" -w "$window_size" -m compress)

      original_file_name=$(basename "$file")
      file_size=$(echo "$output" | grep "Original File Size:" | cut -d ' ' -f 4)
      time_taken_by_brotli=$(echo "$output" | grep "Time taken by Brotli for compression:" | awk '{print $7}' | tr -d 's')
      time_taken_in_compression=$(echo "$output" | grep "Time taken by Full Compression:" | awk '{print $6}' | tr -d 's')
      compressed_file_size=$(echo "$output" | grep "Compressed File Size:" | cut -d ' ' -f 4)
      compression_ratio=$(echo "$output" | grep "Compression Ratio:" | cut -d ' ' -f 3)
      cpu_usage=$(echo "$output" | grep "CPU usage by process:" | cut -d ' ' -f 5 | tr -d '%')
      max_resident_size=$(echo "$output" | grep "Maximum resident set size:" | cut -d ' ' -f 5)
      
      echo "$original_file_name,$file_size,$compression_quality,$window_size,$time_taken_by_brotli,$time_taken_in_compression,$compressed_file_size,$compression_ratio,$cpu_usage,$max_resident_size" >> "$report_file"

      # Move the compressed file to the new directory
      compressed_file="${file}.br"
      if [ -f "$compressed_file" ]; then
        mv "$compressed_file" "$compressed_directory/"
      fi
    done
  fi
done

echo "Compression complete. Report is available in $report_file."