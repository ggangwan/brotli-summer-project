'''

ANNOTATED BAR GRAPH

'''
import pandas as pd
import matplotlib.pyplot as plt

# Define the input file paths
input_files = [
    "/path/to/test_data_compressed_c6_w16/_report_c6_w16.csv",
    "/path/to/test_data_compressed_c6_w16_v110/_report_c6_w16_v110.csv",
    "/path/to/test_data_compressed_c6_w18_v110/_report_c6_w18_v110.csv",
    "/path/to/test_data_compressed_c6_w20_v110/_report_c6_w20_v110.csv",
    "/path/to/test_data_compressed_c6_w22_v110/_report_c6_w22_v110.csv"
]

# Extract the window sizes from the file paths
window_sizes = [file_path.split('_w')[-1][:-4] for file_path in input_files]

# Read the CSV files and store the DataFrames
data_frames = []
for file_path in input_files:
    df = pd.read_csv(file_path)
    data_frames.append(df)

# Group by 'Original File Name' and calculate mean values
mean_data = []
for df in data_frames:
    grouped_df = df.groupby(['Original File Name']).agg({'Time Taken by Brotli(s)': 'mean'})
    mean_data.append(grouped_df)

# Create final DataFrame
final_df = pd.DataFrame()
for i, mean_df in enumerate(mean_data):
    final_df[f'Time_Taken_w{window_sizes[i]}'] = mean_df['Time Taken by Brotli(s)']

final_df['file_name'] = mean_data[0].index

# Plotting the data
fig, ax = plt.subplots(figsize=(15, 10))

bar_width = 0.15
bar_positions = range(len(final_df))

# Plot bars for each window size
for i in range(len(window_sizes)):
    bars = ax.bar([p + i * bar_width for p in bar_positions], final_df[f'Time_Taken_w{window_sizes[i]}'] * 1000, width=bar_width, label=f'Compressed Using Brotli w{window_sizes[i]}')
    
    # Annotate the bars with scaled values (milliseconds)
    for bar in bars:
        height = bar.get_height()
        ax.annotate(f'{height:.0f}', xy=(bar.get_x() + bar.get_width() / 2, height), xytext=(0, 3), textcoords='offset points', ha='center', va='bottom', fontsize=7)

# Set labels and title
plt.xlabel('File Name')
plt.ylabel('Average Time Taken by Brotli (ms)')
plt.title('Average Time Taken by Brotli for Different Window Sizes')

# Set x-ticks and labels
ax.set_xticks([p + 2 * bar_width for p in bar_positions])
ax.set_xticklabels(final_df['file_name'], rotation=45, ha='right')

# Set y-axis scale to milliseconds (10^-3 seconds)
ax.set_ylim(bottom=0, top=ax.get_ylim()[1])

# Set legend
ax.legend()

# Adjust the plot to make room for the text
plt.subplots_adjust(bottom=0.25)

plt.show()

'''

LINE CHART

'''

# import pandas as pd
# import matplotlib.pyplot as plt

# # Define the input file paths
# input_files = [
#     "/path/to/test_data_compressed_c6_w16/_report_c6_w16.csv",
#     "/path/to/test_data_compressed_c6_w16_v110/_report_c6_w16_v110.csv",
#     "/path/to/test_data_compressed_c6_w18_v110/_report_c6_w18_v110.csv",
#     "/path/to/test_data_compressed_c6_w20_v110/_report_c6_w20_v110.csv",
#     "/path/to/test_data_compressed_c6_w22_v110/_report_c6_w22_v110.csv"
# ]

# # Extract the window sizes from the file paths
# window_sizes = [file_path.split('_w')[-1][:-4] for file_path in input_files]

# # Read the CSV files and store the DataFrames
# data_frames = []
# for file_path in input_files:
#     df = pd.read_csv(file_path)
#     data_frames.append(df)

# # Group by 'Original File Name' and calculate mean values
# mean_data = []
# for df in data_frames:
#     grouped_df = df.groupby(['Original File Name']).agg({'Time Taken by Brotli(s)': 'mean'})
#     mean_data.append(grouped_df)

# # Create final DataFrame
# final_df = pd.DataFrame()
# for i, mean_df in enumerate(mean_data):
#     final_df[f'Time_Taken_w{window_sizes[i]}'] = mean_df['Time Taken by Brotli(s)']

# final_df['file_name'] = mean_data[0].index

# # Plotting the data
# fig, ax = plt.subplots(figsize=(12, 8))

# for i in range(len(window_sizes)):
#     final_df.plot(x='file_name', y=f'Time_Taken_w{window_sizes[i]}', kind='line', ax=ax, marker='o')

# # Set labels and title
# plt.xlabel('File Name')
# plt.ylabel('Average Time Taken by Brotli (s)')
# plt.title('Average Time Taken by Brotli for Different Window Sizes')

# # Add space between bars
# ax.set_xticks([i for i in range(len(final_df))])
# ax.set_xticklabels(final_df['file_name'], rotation=45, ha='right')

# # Set legend
# legend_labels = [f'Time_Taken_(s) using w{window_sizes[i]}' for i in range(len(window_sizes))]
# ax.legend(legend_labels)

# # Adjust the plot to make room for the text
# plt.subplots_adjust(bottom=0.3)

# plt.show()