import matplotlib.pyplot as plt
import pandas as pd

# Load and clean data
df = pd.read_csv("../fft_log.csv", header=None,
                 names=["timestamp", "magnitude"], dtype=str)
df = df.dropna()
df = df[df["timestamp"].str.match(r"^\d{2}:\d{2}:\d{2}\.\d{3}$")]
df["magnitude"] = df["magnitude"].astype(float)
df["datetime"] = pd.to_datetime(df["timestamp"], format="%H:%M:%S.%f")

# Compute diff + binary detection
df["dt"] = df["datetime"].diff().dt.total_seconds()

# Magnitude difference
df["dy"] = df["magnitude"].diff()

# Actual slope: magnitude change per second
df["slope"] = df["dy"] / df["dt"]

# Detection based on slope threshold
slope_threshold = 10  # tweak this based on your signal
df["bit"] = (df["slope"] > slope_threshold).fillna(0).astype(int)

# Plot
fig, ax = plt.subplots(figsize=(12, 6))
ax.plot(df["datetime"], df["magnitude"], marker='o',
        linestyle='-', label="Magnitude")
ax.step(df["datetime"], df["bit"] * df["magnitude"].max(),
        color='red', label="Detected 1s")

# Force every timestamp label
ax.set_xticks(df["datetime"])
ax.set_xticklabels(df["timestamp"], rotation=90)

ax.set_xlabel("Time (HH:MM:SS.mmm)")
ax.set_ylabel("FFT Magnitude")
ax.set_title("Magnitude + 1 Detection (Rate-Based)")
ax.legend()
ax.grid(True)
plt.tight_layout()
plt.show()
