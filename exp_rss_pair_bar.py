import pandas as pd
import matplotlib.pyplot as plt

# Define the input file paths
input_file_1 = "/Users/ggangwan/Documents/brotli-test/test_data_new_compressed_c6_w16/_report_c6_w16.csv"
input_file_2 = "/Users/ggangwan/Documents/brotli-test/test_data_new_compressed_c6_w20/_report_c6_w20.csv"

# Extract window sizes from directory names
window_size_1 = input_file_1.split('/')[-2][25:]
window_size_2 = input_file_2.split('/')[-2][25:]

# Read the CSV files
df_first = pd.read_csv(input_file_1)
df_second = pd.read_csv(input_file_2)

# Group by 'Original File Name' and calculate mean values
gb_first = df_first.groupby('Original File Name')['Maximum Resident Size(B)'].mean().reset_index()
gb_second = df_second.groupby('Original File Name')['Maximum Resident Size(B)'].mean().reset_index()

# Merge the two dataframes on 'Original File Name'
merged_df = pd.merge(gb_first, gb_second, on='Original File Name', suffixes=('_1', '_2'))

# Calculate percentage change
merged_df['percentage_change'] = (merged_df['Maximum Resident Size(B)_1'] - merged_df['Maximum Resident Size(B)_2']) * 100 / merged_df['Maximum Resident Size(B)_1']

# Sort by original file size
original_sizes = df_first[['Original File Name', 'File Size(B)']].drop_duplicates().set_index('Original File Name')
merged_df = merged_df.join(original_sizes, on='Original File Name')
merged_df = merged_df.sort_values(by='File Size(B)')

# Plotting the data
fig, ax = plt.subplots(figsize=(12, 8))

# Plot the bars
bar_width = 0.3
x = range(len(merged_df))

# Plot the maximum resident size bars
ax.bar([p - bar_width for p in x], merged_df['Maximum Resident Size(B)_1'] / (1024**2), width=bar_width, label='Max Resident Size 1 (MB)')
ax.bar(x, merged_df['Maximum Resident Size(B)_2'] / (1024**2), width=bar_width, label='Max Resident Size 2 (MB)')
ax.bar([p + bar_width for p in x], merged_df['percentage_change'], width=bar_width, label='Percentage Change (%)', color='gray', alpha=0.5)

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
ax.set_ylabel("Maximum Resident Size (MB)")

# Enable grid
ax.grid(True)

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