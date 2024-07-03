import pandas as pd
import matplotlib.pyplot as plt

# Define the input file paths
input_file_1 = "/path/to/first_report.csv"
input_file_2 = "/path/to/second_report.csv"

# Read the CSV files
df_first = pd.read_csv(input_file_1)
df_second = pd.read_csv(input_file_2)

# Group by 'Original File Name' and calculate mean values
mean_data_first = df_first.groupby('Original File Name')['Time Taken by Brotli(s)'].mean().reset_index()
mean_data_second = df_second.groupby('Original File Name')['Time Taken by Brotli(s)'].mean().reset_index()

# Merge the two dataframes on 'Original File Name'
merged_df = pd.merge(mean_data_first, mean_data_second, on='Original File Name', suffixes=('_1', '_2'))

# Calculate percentage change
merged_df['percentage_change'] = (merged_df['Time Taken by Brotli(s)_2'] - merged_df['Time Taken by Brotli(s)_1']) * 100 / merged_df['Time Taken by Brotli(s)_1']

# Plotting the data
fig, ax = plt.subplots(figsize=(12, 8))

# Plot the bars
bar_width = 0.3
x = range(len(merged_df))

# Plot the time bars
ax.bar([p - bar_width for p in x], merged_df['Time Taken by Brotli(s)_1'] * 1000, width=bar_width, label='Compressed Using Brotli Time 1 (ms)')
ax.bar(x, merged_df['Time Taken by Brotli(s)_2'] * 1000, width=bar_width, label='Compressed Using Brotli Time 2 (ms)')
ax.bar([p + bar_width for p in x], merged_df['percentage_change'], width=bar_width, label='Percentage Change (%)', color='gray', alpha=0.5)

# Annotate the bars for Time Taken by Brotli(s)
for i, row in merged_df.iterrows():
    ax.annotate(f"{row['Time Taken by Brotli(s)_1'] * 1000:.1f}", (i - bar_width, row['Time Taken by Brotli(s)_1'] * 1000), ha='center', va='bottom', fontsize=8)
    ax.annotate(f"{row['Time Taken by Brotli(s)_2'] * 1000:.1f}", (i, row['Time Taken by Brotli(s)_2'] * 1000), ha='center', va='bottom', fontsize=8)
    ax.annotate(f"{row['percentage_change']:.2f}%", (i + bar_width, row['percentage_change']), ha='center', va='bottom', fontsize=8)

# Set x-axis labels
ax.set_xticks(x)
ax.set_xticklabels(merged_df['Original File Name'], rotation=45, ha='right')

# Calculate percentiles
percentiles = [10, 20, 50, 90, 99]
percentile_values = merged_df['percentage_change'].quantile([p / 100 for p in percentiles])

# Average percentage change
average_percentage_change = merged_df['percentage_change'].mean()
percentile_info = ' | '.join([f"{p}th Percentile: {percentile_values[p / 100]:.2f}%" for p in percentiles])

# Display text
plt.figtext(0.5, 0.9, f"Average Percentage Change: {average_percentage_change:.2f}%\n{percentile_info}", ha="center", fontsize=12, color="black")

# Set y-axis label
ax.set_ylabel("Time Taken By Brotli (ms)")

# Set legend
ax.legend()

# Adjust the plot to make room for the text
plt.subplots_adjust(top=0.85, bottom=0.3)

plt.show()
