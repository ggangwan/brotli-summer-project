#!/bin/bash

# Check if the correct number of arguments is provided
if [ "$#" -ne 2 ]; then
  echo "Usage: $0 <directory> <string_to_append>"
  exit 1
fi

# Assign input arguments to variables
directory=$1
string_to_append=$2

# Check if the provided directory exists
if [ ! -d "$directory" ]; then
  echo "Error: Directory $directory does not exist."
  exit 1
fi

# Loop through all files in the directory
for file in "$directory"/*; do
  # Check if it's a file (not a directory)
  if [ -f "$file" ]; then
    # Get the base name of the file
    base_name=$(basename "$file")
    # Construct the new file name with the string appended
    new_name="$directory/$string_to_append$base_name"
    # Rename the file
    mv "$file" "$new_name"
  fi
done

echo "All files have been renamed."
