import pandas as pd
import matplotlib.pyplot as plt

# Define the input file paths
input_file_1 = "/Users/ggangwan/Documents/brotli-test/test_data_compressed_c6_w16/_report_c6_w16.csv"
input_file_2 = "/Users/ggangwan/Documents/brotli-test/test_data_compressed_c6_w24_v110/_report_c6_w24_v110.csv"

# Extract the window sizes from the file paths
window_size_1 = input_file_1.split('_w')[-1][:-4]
window_size_2 = input_file_2.split('_w')[-1][:-4]

# Read the CSV files
df_first = pd.read_csv(input_file_1)
df_first[f'compressed_size(KB)_w{window_size_1}'] = df_first['Compressed File Size(B)'] / 1024.0

df_second = pd.read_csv(input_file_2)
df_second[f'compressed_size(KB)_w{window_size_2}'] = df_second['Compressed File Size(B)'] / 1024.0

# Print for verification
print(df_first[f'compressed_size(KB)_w{window_size_1}'])
print(df_second[f'compressed_size(KB)_w{window_size_2}'])

# Group by 'Original File Name'
gb_first = df_first.groupby(['Original File Name'])
gb_second = df_second.groupby(['Original File Name'])

# Calculate mean values
mean_data_first = gb_first.agg({'Time Taken by Brotli(s)': 'mean', f'compressed_size(KB)_w{window_size_1}': 'mean'})
mean_data_second = gb_second.agg({'Time Taken by Brotli(s)': 'mean', f'compressed_size(KB)_w{window_size_2}': 'mean'})

# Create final DataFrame
final_df = pd.DataFrame({
    f'compressed_size_w{window_size_1}': mean_data_first[f'compressed_size(KB)_w{window_size_1}'],
    f'compressed_size_w{window_size_2}': mean_data_second[f'compressed_size(KB)_w{window_size_2}']
})
final_df['percentage_change'] = -(final_df[f'compressed_size_w{window_size_2}'] - final_df[f'compressed_size_w{window_size_1}']) * 100 / final_df[f'compressed_size_w{window_size_1}']
final_df['file_name'] = final_df.index

# Plotting the data
fig, ax = plt.subplots(figsize=(12, 8))
final_df.plot(x='file_name', y=[f'compressed_size_w{window_size_1}', f'compressed_size_w{window_size_2}', 'percentage_change'], kind="bar", width=0.8, ax=ax)

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

# Average percentage change
average_percentage_change = final_df['percentage_change'].mean()
plt.figtext(0.5, 0.02, f"Average Percentage Change: {average_percentage_change:.2f}%", ha="center", fontsize=12, color="black")

# Set legend
legend_labels = [
    f'Compressed_Size_(KB) using w{window_size_1}',
    f'Compressed_Size_(KB) using w{window_size_2}',
    'Percentage Change'
]
ax.legend(legend_labels)

# Adjust the plot to make room for the text
plt.subplots_adjust(bottom=0.3)

plt.show()
