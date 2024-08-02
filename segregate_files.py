import os
import shutil
import sys

def get_size_bucket(file_size):
    if file_size < 64 * 1024:
        return "64"
    bucket_start = 64
    while bucket_start * 1024 <= file_size:
        bucket_start *= 2
    bucket_end = bucket_start
    bucket_start //= 2
    return f"{bucket_start}-{bucket_end}"

def create_buckets(directory):
    files = [f for f in os.listdir(directory) if os.path.isfile(os.path.join(directory, f))]
    print(f"Total files found: {len(files)}")
    bucket_dict = {}
    for file in files:
        file_path = os.path.join(directory, file)
        file_size = os.path.getsize(file_path)
        bucket = get_size_bucket(file_size)
        if bucket not in bucket_dict:
            bucket_dict[bucket] = []
        bucket_dict[bucket].append(file_path)
    return bucket_dict

def copy_files_to_buckets(directory, bucket_dict):
    parent_directory = os.path.dirname(directory)
    total_files_copied = 0
    for bucket, files in bucket_dict.items():
        bucket_dir = os.path.join(parent_directory, bucket)
        os.makedirs(bucket_dir, exist_ok=True)
        for file_path in files:
            shutil.copy(file_path, bucket_dir)
            total_files_copied += 1
    print(f"Total files copied: {total_files_copied}")

def main():
    if len(sys.argv) != 2:
        print("Usage: python script.py <directory>")
        sys.exit(1)
    
    directory = sys.argv[1]

    if not os.path.isdir(directory):
        print(f"Directory {directory} does not exist.")
        sys.exit(1)

    bucket_dict = create_buckets(directory)
    copy_files_to_buckets(directory, bucket_dict)
    print("Files have been segregated and copied to respective buckets successfully.")

if __name__ == "__main__":
    main()