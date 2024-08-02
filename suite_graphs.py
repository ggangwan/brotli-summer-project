import os
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

def read_report_data(report_file):
    df = pd.read_csv(report_file)
    return df.groupby('Original File Name').mean()

def plot_and_save(data, output_path, y_label, title, plot_file):
    plt.figure(figsize=(10, 6))
    ax = data.plot(kind='bar', width=0.8)
    
    # Annotate bars with values
    for p in ax.patches:
        ax.annotate(f'{p.get_height():.2f}', 
                    (p.get_x() + p.get_width() / 2., p.get_height()), 
                    ha='center', va='center', xytext=(0, 10), 
                    textcoords='offset points', fontsize=8)
    
    plt.ylabel(y_label)
    plt.title(title)
    plt.tight_layout()
    plt.savefig(plot_file)
    plt.close()

def process_suite_directory(suite_directory):
    files_data = {}
    
    # Collect data from all report files
    for dirpath, dirnames, filenames in os.walk(suite_directory):
        for filename in filenames:
            if filename.startswith('_report') and filename.endswith('.csv'):
                report_file = os.path.join(dirpath, filename)
                df = read_report_data(report_file)
                compression_level, window_bits = parse_compression_params(filename)
                for file_name, row in df.iterrows():
                    if file_name not in files_data:
                        files_data[file_name] = []
                    files_data[file_name].append((compression_level, window_bits, row))

    # Process each file
    for file_name, entries in files_data.items():
        output_dir = os.path.join(suite_directory, file_name)
        os.makedirs(output_dir, exist_ok=True)
        
        compressed_sizes = {}
        times_taken = {}
        
        for compression_level, window_bits, row in entries:
            key = f'c{compression_level}_w{window_bits}'
            compressed_sizes[key] = row['Compressed File Size(B)']
            times_taken[key] = row['Time Taken by Brotli(s)'] * 1000  # Convert to ms
            
        # Plot compressed sizes
        compressed_sizes_series = pd.Series(compressed_sizes)
        plot_and_save(
            compressed_sizes_series,
            output_dir,
            'Compressed Size (Bytes)',
            f'Compressed Size for {file_name}',
            os.path.join(output_dir, 'compressed_size.png')
        )
        
        # Plot times taken
        times_taken_series = pd.Series(times_taken)
        plot_and_save(
            times_taken_series,
            output_dir,
            'Time Taken by Brotli (ms)',
            f'Time Taken by Brotli for {file_name}',
            os.path.join(output_dir, 'time_taken.png')
        )

def parse_compression_params(filename):
    parts = filename.split('_')
    compression_level = int(parts[2][1])
    window_bits = int(parts[3][1:3])
    return compression_level, window_bits

if __name__ == "__main__":
    suite_directory = input("Enter the path to the suite directory: ")
    if not os.path.isdir(suite_directory):
        print(f"Directory {suite_directory} does not exist.")
        exit(1)
    
    process_suite_directory(suite_directory)
    print("Plots created successfully.")
