import pandas as pd
import matplotlib.pyplot as plt

# Define the input file paths
input_file_1 = "/Users/ggangwan/Documents/brotli-test/test_data_new_compressed_c6_w16/_report_c6_w16.csv"
input_file_2 = "/Users/ggangwan/Documents/brotli-test/test_data_new_compressed_c6_w18/_report_c6_w18.csv"

# Extract window sizes from directory names
window_size_1 = input_file_1.split('/')[-2][25:]
window_size_2 = input_file_2.split('/')[-2][25:]

# Read the CSV files
df_first = pd.read_csv(input_file_1)
df_second = pd.read_csv(input_file_2)

# Group by 'Original File Name' and calculate mean values for compressed size
gb_first_size = df_first.groupby('Original File Name')['Compressed File Size(B)'].mean().reset_index()
gb_second_size = df_second.groupby('Original File Name')['Compressed File Size(B)'].mean().reset_index()

# Get the original file sizes
gb_first_orig_size = df_first.groupby('Original File Name')['File Size(B)'].first().reset_index()

# Merge the compressed size and original size dataframes
merged_df = pd.merge(gb_first_size, gb_second_size, on='Original File Name', suffixes=('_1', '_2'))
merged_df = pd.merge(merged_df, gb_first_orig_size, on='Original File Name')

# Sort by 'File Size(B)'
merged_df = merged_df.sort_values(by='File Size(B)')

# Calculate percentage change in compressed size
merged_df['percentage_change'] = (merged_df['Compressed File Size(B)_1'] - merged_df['Compressed File Size(B)_2']) * 100 / merged_df['Compressed File Size(B)_1']

# Plotting the data
fig, ax = plt.subplots(figsize=(12, 8))

# Plot the lines
x = range(len(merged_df))

# Plot the compressed size lines
ax.plot(x, merged_df['Compressed File Size(B)_1'] / 1024, marker='o', label='Compressed Using Brotli Size 1 (KB)')
ax.plot(x, merged_df['Compressed File Size(B)_2'] / 1024, marker='o', label='Compressed Using Brotli Size 2 (KB)')
ax.plot(x, merged_df['percentage_change'], marker='o', label='Percentage Change (%)', color='gray', alpha=0.5)

# Set x-axis labels
ax.set_xticks(x)
ax.set_xticklabels(merged_df['Original File Name'], rotation=45, ha='right')

# Calculate percentiles
percentiles = [10, 20, 50, 90, 99]
percentile_values = merged_df['percentage_change'].quantile([p/100 for p in percentiles])

# Average percentage change
average_percentage_change = merged_df['percentage_change'].mean()
percentile_info = ' | '.join([f"{p}th Percentile: {percentile_values[p/100]:.2f}%" for p in percentiles])

# Display text
plt.figtext(0.5, 0.9, f"Average Percentage Change: {average_percentage_change:.2f}%\n{percentile_info}", ha="center", fontsize=12, color="black")

# Set y-axis label
ax.set_ylabel("Compressed Size (KB)")

# Set legend
legend_labels = [
    f'Compressed Using {window_size_1}',
    f'Compressed Using {window_size_2}',
    'Percentage Change'
]
ax.legend(legend_labels)

# Adjust the plot to make room for the text
plt.subplots_adjust(top=0.85, bottom=0.3)

plt.show()