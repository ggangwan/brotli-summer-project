import os
import pandas as pd
import matplotlib.pyplot as plt

# Define the input directory paths
input_dir_1 = "/Users/ggangwan/Documents/brotli-test/test_data_compressed_c6_w16/"
input_dir_2 = "/Users/ggangwan/Documents/brotli-test/test_data_compressed_c6_w22_v110/"

# Extract window sizes from directory names
window_size_1 = input_dir_1.split('compressed_')[-1][:-1]
window_size_2 = input_dir_2.split('compressed_')[-1][:-1]

# Function to calculate compressed file sizes
def calculate_compressed_sizes(directory):
    compressed_sizes = {}
    for filename in os.listdir(directory):
        if filename.endswith('.br'):  # Assuming compressed files have .br extension
            file_path = os.path.join(directory, filename)
            file_size = os.path.getsize(file_path) / 1024.0  # Convert bytes to KB
            compressed_sizes[filename] = file_size
    return compressed_sizes

# Calculate compressed sizes for both directories
compressed_sizes_1 = calculate_compressed_sizes(input_dir_1)
compressed_sizes_2 = calculate_compressed_sizes(input_dir_2)

# Create DataFrames from dictionaries
df_first = pd.DataFrame(list(compressed_sizes_1.items()), columns=['file_name', f'compressed_size_{window_size_1}'])
df_second = pd.DataFrame(list(compressed_sizes_2.items()), columns=['file_name', f'compressed_size_{window_size_2}'])

# Merge DataFrames on 'file_name'
final_df = pd.merge(df_first, df_second, on='file_name')

# Calculate percentage change
final_df['percentage_change'] = -(final_df[f'compressed_size_{window_size_2}'] - final_df[f'compressed_size_{window_size_1}']) * 100 / final_df[f'compressed_size_{window_size_1}']

# Plotting the data
fig, ax = plt.subplots(figsize=(12, 8))
final_df.plot(x='file_name', y=[f'compressed_size_{window_size_1}', f'compressed_size_{window_size_2}', 'percentage_change'], kind="bar", width=0.8, ax=ax)

# Annotate the bars
x_offset = -0.03
y_offset = 0.02

for p in ax.patches:
    b = p.get_bbox()
    val = "{:.1f}".format(b.y1 - b.y0)  # Corrected annotation value to reflect height of the bar
    ax.annotate(val, ((b.x0 + b.x1) / 2 + x_offset, b.y1 + y_offset), ha='center')

# Add space between bars
ax.set_xticks([i for i in range(len(final_df))])
ax.set_xticklabels(final_df['file_name'], rotation=45, ha='right')

# Calculate percentiles
percentiles = [10, 20, 50, 90, 99]
percentile_values = final_df['percentage_change'].quantile([p/100 for p in percentiles])

# Average percentage change
average_percentage_change = final_df['percentage_change'].mean()
percentile_info = ' | '.join([f"{p}th Percentile: {percentile_values[p/100]:.2f}%" for p in percentiles])

# Display text
plt.figtext(0.5, 0.9, f"Average Percentage Change: {average_percentage_change:.2f}%\n{percentile_info}", ha="center", fontsize=12, color="black")

# Set legend
legend_labels = [
    f'Compressed_Size_(KB) using {window_size_1}',
    f'Compressed_Size_(KB) using {window_size_2}',
    'Percentage Change'
]
ax.legend(legend_labels)

# Adjust the plot to make room for the text
plt.subplots_adjust(top=0.85, bottom=0.15)

plt.show()
