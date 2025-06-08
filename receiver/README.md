# Covert Channel Receiver

## Overview
A Raspberry Pi-based RF receiver that decodes signals to (display burger ingredients). Uses HackRF for signal reception and FFT processing for decoding.

## Requirements
- Hardware: Raspberry Pi 4, HackRF One
- Software: FFTW3, HackRF, SDL2

## Build
```bash
# Install dependencies
sudo apt-get install libfftw3-dev libhackrf-dev libsdl2-dev libsdl2-image-dev

# Compile
make
```

## Run
```bash
./receiver
```

## How it Works
- Receives RF signals at 63MHz
- Uses FFT to analyze signal strength
- Decodes bits based on signal changes
- Displays burger ingredients as they're received

## Technical Details
### Signal Processing
- Sample Rate: 8MHz
- Center Frequency: 63MHz
- FFT Size: 262144 samples
- Frequency Range: 61.5MHz - 61.55MHz
- Bits per second: 2 (0.5MHz transmitter rate)
- Modulation: OOK (On-Off Keying)

### Bit Detection
- Uses signal magnitude changes to detect bits
- Thresholds: +15% for 1, -15% for 0
- Averages 8 FFT readings per bit
- Concatenates 2 FFTs for noise reduction

### Timing & Synchronization
- Transmitter sends at exact second boundaries
- Receiver starts 100ms after each second
- Accepts 10ms jitter window at start of each second
- Takes 8 FFT readings per bit (0.5MHz/8 = 62.5kHz pause between reads)
- Uses precise timing to maintain bit synchronization

### Calibration
- Pattern: 1,0,1,0,1,0,1,0
- Synchronizes with transmitter
- Starts decoding after successful calibration

## Troubleshooting
- If HackRF isn't detected: `hackrf_info` to verify connection
- If compilation fails: Check all dependencies are installed
- If no signal: Verify transmitter is on and at correct frequency

## Notes
- Calibration pattern: 1,0,1,0,1,0,1,0
- Sample rate: 8MHz
- Center frequency: 63MHz 