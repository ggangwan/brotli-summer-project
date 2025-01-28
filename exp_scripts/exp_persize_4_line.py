import pandas as pd
import matplotlib.pyplot as plt

# Define the input file paths
input_file_1 = "/Users/ggangwan/Documents/brotli-test/test_data_new_compressed_c6_w16/_report_c6_w16.csv"
input_file_2 = "/Users/ggangwan/Documents/brotli-test/test_data_new_compressed_c6_w18/_report_c6_w18.csv"
input_file_3 = "/Users/ggangwan/Documents/brotli-test/test_data_new_compressed_c6_w16_v110/_report_c6_w16_v110.csv"
input_file_4 = "/Users/ggangwan/Documents/brotli-test/test_data_new_compressed_c6_w18_v110/_report_c6_w18_v110.csv"

# Extract window sizes from directory names
window_size_1 = input_file_1.split('/')[-2][25:]
window_size_2 = input_file_2.split('/')[-2][25:]
window_size_3 = input_file_3.split('/')[-2][25:]
window_size_4 = input_file_4.split('/')[-2][25:]

# Read the CSV files
df_first = pd.read_csv(input_file_1)
df_second = pd.read_csv(input_file_2)
df_third = pd.read_csv(input_file_3)
df_fourth = pd.read_csv(input_file_4)

# Group by 'Original File Name' and calculate mean values for compressed size
gb_first_size = df_first.groupby('Original File Name')['Compressed File Size(B)'].mean().reset_index()
gb_second_size = df_second.groupby('Original File Name')['Compressed File Size(B)'].mean().reset_index()
gb_third_size = df_third.groupby('Original File Name')['Compressed File Size(B)'].mean().reset_index()
gb_fourth_size = df_fourth.groupby('Original File Name')['Compressed File Size(B)'].mean().reset_index()

# Get the original file sizes
gb_first_orig_size = df_first.groupby('Original File Name')['File Size(B)'].first().reset_index()

# Merge the compressed size and original size dataframes
merged_df = pd.merge(gb_first_size, gb_second_size, on='Original File Name', suffixes=('_1', '_2'))
merged_df = pd.merge(merged_df, gb_third_size, on='Original File Name')
merged_df = pd.merge(merged_df, gb_fourth_size, on='Original File Name', suffixes=('_3', '_4'))
merged_df = pd.merge(merged_df, gb_first_orig_size, on='Original File Name')

# Sort by 'File Size(B)'
merged_df = merged_df.sort_values(by='File Size(B)')

# Calculate percentage improvement compared to the first entry
def calculate_percentage_improvement(df, col):
    return (df[col] - df['Compressed File Size(B)_1']) * 100 / df['Compressed File Size(B)_1']

merged_df['improvement_2'] = -calculate_percentage_improvement(merged_df, 'Compressed File Size(B)_2')
merged_df['improvement_3'] = -calculate_percentage_improvement(merged_df, 'Compressed File Size(B)_3')
merged_df['improvement_4'] = -calculate_percentage_improvement(merged_df, 'Compressed File Size(B)_4')

# Plotting the data
fig, ax = plt.subplots(figsize=(12, 8))

# Plot the improvement lines with smaller markers
x = range(len(merged_df))

# Plot the percentage improvement lines
ax.plot(x, merged_df['improvement_2'], marker='o', markersize=3, label=f'Improvement Using {window_size_2} (%)')
ax.plot(x, merged_df['improvement_3'], marker='o', markersize=3, label=f'Improvement Using {window_size_3} (%)')
ax.plot(x, merged_df['improvement_4'], marker='o', markersize=3, label=f'Improvement Using {window_size_4} (%)')

# Set x-axis labels
ax.set_xticks(x)
ax.set_xticklabels(merged_df['Original File Name'], rotation=45, ha='right')

# Calculate percentiles for each percentage improvement
percentiles = [10, 20, 50, 90, 99]
percentile_values_2 = merged_df['improvement_2'].quantile([p/100 for p in percentiles])
percentile_values_3 = merged_df['improvement_3'].quantile([p/100 for p in percentiles])
percentile_values_4 = merged_df['improvement_4'].quantile([p/100 for p in percentiles])

# Average percentage improvement
average_improvement_2 = merged_df['improvement_2'].mean()
average_improvement_3 = merged_df['improvement_3'].mean()
average_improvement_4 = merged_df['improvement_4'].mean()

percentile_info_2 = ' | '.join([f"{p}th Percentile: {percentile_values_2[p/100]:.2f}%" for p in percentiles])
percentile_info_3 = ' | '.join([f"{p}th Percentile: {percentile_values_3[p/100]:.2f}%" for p in percentiles])
percentile_info_4 = ' | '.join([f"{p}th Percentile: {percentile_values_4[p/100]:.2f}%" for p in percentiles])

# Display text
plt.figtext(0.5, 0.86, f"Avg % Improvement {window_size_2}: {average_improvement_2:.2f}%\n{percentile_info_2}\n"
                      f"Avg % Improvement {window_size_3}: {average_improvement_3:.2f}%\n{percentile_info_3}\n"
                      f"Avg % Improvement {window_size_4}: {average_improvement_4:.2f}%\n{percentile_info_4}", 
            ha="center", fontsize=12, color="black")

# Set y-axis label
ax.set_ylabel("Percentage Improvement (%)")

# Set legend
legend_labels = [
    f'Improvement Using {window_size_2}',
    f'Improvement Using {window_size_3}',
    f'Improvement Using {window_size_4}'
]
ax.legend(legend_labels)


# Adjust the plot to make room for the text and utilize space more effectively
plt.subplots_adjust(top=0.85, bottom=0.2)

plt.show()
