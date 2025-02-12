import numpy as np
import matplotlib.pyplot as plt
from scipy.optimize import curve_fit

def power_law(x, a, b):
    return a * np.power(x, b)

def fit_and_plot(dpu_counts, dpu_run_times, inter_dpu_times):
    """
    Fits a power law function to the data using log-log transformation and plots the relationship between DPUs and total time (Run Time + Inter-DPU Time).
    
    Parameters:
        dpu_counts (list): List of DPU numbers.
        dpu_run_times (list): Corresponding DPU run times.
        inter_dpu_times (list): Corresponding inter-DPU times.
    """
    dpu_counts = np.array(dpu_counts)
    total_times = np.array(dpu_run_times) + np.array(inter_dpu_times)
    
    # Apply log transformation
    log_x = np.log(dpu_counts)
    log_y = np.log(total_times)
    
    # Fit a linear model to log-log data
    coeffs = np.polyfit(log_x, log_y, 1)  # Linear fit in log-log space
    b = coeffs[0]  # Slope (power exponent)
    a = np.exp(coeffs[1])  # Convert intercept back from log space
    
    # Generate smooth x values for plotting
    x_smooth = np.linspace(min(dpu_counts), max(dpu_counts), 100)
    y_total_fit = power_law(x_smooth, a, b)
    
    plt.figure(figsize=(10, 5))
    
    # Plot actual data
    plt.scatter(dpu_counts, total_times, color='purple', label='Total Time (Actual)')
    
    # Plot fitted function
    plt.plot(x_smooth, y_total_fit, 'm--', label=f'Fit: {a:.2e} * x^{b:.2f}')
    
    plt.xscale("log")  # Use log scale for better visualization
    plt.yscale("log")
    plt.xlabel('Number of DPUs')
    plt.ylabel('Total Time (µs)')
    plt.title('DPU Number vs. Running Time + Inter-DPU Time (Power Law Fit)')
    plt.legend()
    plt.grid(True, linestyle='--', linewidth=0.5)
    plt.savefig('bar.png')
    
    # Print the regression equation
    print(f'Power Law Equation: Total Time = {a:.2e} * DPUs^{b:.2f}')

# Data extracted from the benchmark
dpu_counts = [1, 2, 4, 8, 16, 32, 64, 128, 256, 512]
dpu_run_times = [394895.416, 197483.966, 98635.911, 49313.392, 25141.189, 12378.411, 6182.441, 3091.354, 1573.473, 815.165]
inter_dpu_times = [317870.005, 237853.874, 116936.599, 56881.567, 43593.283, 33225.756, 32930.789, 32895.336, 19431.199, 19269.901]

# Call the function to fit and plot
fit_and_plot(dpu_counts, dpu_run_times, inter_dpu_times)

    
    
   