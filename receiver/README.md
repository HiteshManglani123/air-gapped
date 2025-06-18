# Covert Channel Receiver

## Overview
A Raspberry Pi-based RF receiver that decodes covert signals to display burger ingredients in real-time. This system demonstrates wireless covert communication by receiving and decoding RF signals transmitted at 63MHz, then displaying the decoded ingredients on a visual interface.

## How It Works
The receiver captures electromagnetic signals using a HackRF software-defined radio, processes them through FFT analysis to detect signal strength changes (OOK Modulation), and decodes the binary data into burger ingredients using packets of (4) bits. The system uses timing synchronization and calibration to maintain reliable communication with the transmitter.

### Key Features
- **Real-time signal processing** at 8MHz sample rate
- **Automatic calibration** with sliding window pattern matching
- **Bit decoding** using magnitude threshold analysis
- **Visual feedback** through SDL2 graphics interface
- **Timing synchronization** for reliable communication with the transmission

## Requirements
- **Hardware**: Raspberry Pi 4, HackRF One
- **Software**: FFTW3, HackRF, SDL2

## Build & Run
```bash
# Install dependencies
sudo apt-get install libfftw3-dev libhackrf-dev libsdl2-dev libsdl2-image-dev

# Compile
make

# Run
./receiver
```

## Testing
Run the test suite to validate core algorithms:
```bash
cd tests
make test
```

**Tested Components:**
- Timing synchronization (pairing logic)
- Calibration algorithm (pattern matching with error recovery)
- Bit decoding (threshold-based bit detection)
- Packet reconstruction (bit-to-byte conversion)

## Technical Specifications

### Signal Processing
- **Sample Rate**: 8MHz
- **Center Frequency**: 63MHz
- **FFT Size**: 262144 samples
- **Frequency Range**: 61.5MHz - 61.55MHz
- **Data Rate**: 2 bits per second
- **Modulation**: OOK (On-Off Keying)

### Bit Detection Algorithm
- Detects bits based on signal magnitude changes
- **Thresholds**: +15% increase for bit '1', -15% decrease for bit '0'
- Averages 8 FFT readings per bit for noise reduction
- Uses concatenated FFTs for improved signal quality

### Synchronization System
- **Transmitter timing**: Sends at exact second boundaries
- **Receiver timing**: Starts 100ms after each second
- **Jitter tolerance**: 10ms window at start of each second
- **Bit synchronization**: 8 FFT readings per bit (62.5kHz intervals)

### Calibration Protocol
- **Pattern**: 1,0,1,0,1,0,1,0 (8-bit sequence)
- **Method**: Sliding window pattern matching
- **Recovery**: Automatic reset on mismatch, continues from current bit
- **Completion**: Starts data decoding after successful calibration
