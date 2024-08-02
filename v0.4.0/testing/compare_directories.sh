#!/bin/bash

# Check if the correct number of arguments is passed
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <directory1> <directory2>"
    exit 1
fi

# Input directories
dir1="$1"
dir2="$2"

# Check if both directories exist
if [ ! -d "$dir1" ] || [ ! -d "$dir2" ]; then
    echo "One or both directories do not exist."
    exit 1
fi

# Get lists of files in both directories
files1=($(ls -1 "$dir1"))
files2=($(ls -1 "$dir2"))

# Check if the number of files is the same
if [ "${#files1[@]}" -ne "${#files2[@]}" ]; then
    echo "The directories do not contain the same number of files."
    exit 1
fi

# Check if filenames are the same and in the same order
for i in "${!files1[@]}"; do
    if [ "${files1[$i]}" != "${files2[$i]}" ]; then
        echo "File mismatch: ${files1[$i]} != ${files2[$i]}"
        exit 1
    fi
done

# Flag to track if any differences are found
differences_found=false

# Perform diff for each file
for i in "${!files1[@]}"; do
    file1="${dir1}/${files1[$i]}"
    file2="${dir2}/${files2[$i]}"
    diff "$file1" "$file2" > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo "Files $file1 and $file2 differ."
        differences_found=true
    fi
done

if [ "$differences_found" = false ]; then
    echo "All files are identical."
fi

echo "Comparison complete."
