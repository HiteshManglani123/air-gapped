import sys

f = open(sys.argv[1], "r")
input_data = ''.join(f.readlines())

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# Function to parse the data into a DataFrame
def parse_data(input_data):
    # Split the data into rows and columns
    rows = input_data.strip().split('\n')
    columns = ['Date', 'Timestamp', 'StartFreq', 'EndFreq', 'ResolutionBandwidth', 'Amplitude', 'Freq1', 'Freq2', 'Freq3', 'Freq4', 'Freq5']
    
    # Parse each row into the DataFrame
    data = [row.split(', ') for row in rows]
    df = pd.DataFrame(data, columns=columns)
    
    # Convert numeric columns to float where applicable
    for col in ['StartFreq', 'EndFreq', 'ResolutionBandwidth', 'Amplitude', 'Freq1', 'Freq2', 'Freq3', 'Freq4', 'Freq5']:
        df[col] = pd.to_numeric(df[col], errors='coerce')
    
    return df

# Function to plot the waterfall graph
def plot_waterfall(df):
    # Extract frequency data for plotting (columns Freq1, Freq2, Freq3, Freq4, Freq5)
    freq_columns = ['Freq1', 'Freq2', 'Freq3', 'Freq4', 'Freq5']
    
    # Convert the data into a matrix for waterfall plotting
    data_matrix = df[freq_columns].values.T  # Transpose to get each frequency as a line in the plot
    
    # Generate time steps based on the row index (time is not plotted but used as an index)
    time_steps = np.arange(data_matrix.shape[1])
    
    # Create the waterfall plot
    plt.figure(figsize=(12, 8))
    for i, freq_data in enumerate(data_matrix):
        plt.plot(time_steps, freq_data, label=f'Frequency {i+1}')
    
    plt.title('Waterfall Plot of Spectrum Analyzer Output')
    plt.xlabel('Time (Index)')
    plt.ylabel('Frequency (dB)')
    plt.legend()
    plt.grid(True)
#    plt.tight_layout()
    plt.show()

# Parse the data and plot
df = parse_data(input_data)
plot_waterfall(df)

