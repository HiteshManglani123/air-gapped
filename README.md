# Air-Gapped Communication Research Project

![C](https://img.shields.io/badge/C-00599C?style=for-the-badge&logo=c&logoColor=white)
![Python](https://img.shields.io/badge/Python-3776AB?style=for-the-badge&logo=python&logoColor=white)

A research project exploring various methods of covert communication across air-gapped systems, with a focus on electromagnetic emissions channels. 

## 🏗️ Project Structure

The project is organized into three main components:

### 1. CS Research (`/cs_research`)

Contains multiple proof-of-concept implementations including:
- **Encoder/Decoder Simulator**: RF transmission system using frequency sweeps
- **CPU-based EM Covert Channel**: Electromagnetic emissions through CPU load modulation
- **4-FSK Audio Channel**: Covert communication using audio frequencies
- **Working Pair**: A complete implementation of Adaptive Covert Electromagnetic Transmission (ACET)

### 2. Receiver (`/receiver`)

The receiving component of the system, featuring:
- GUI implementation with burger stack visualization
- HackRF integration for SDR capabilities
- FFT-based signal processing and analysis
- Unit Testing

### 3. Transmitter (`/transmitter`)

The transmission component, including:
- Various attack techniques implementations
- CPU-based transmission methods
- Power supply manipulation techniques

## 🔬 Key Features

- Multiple covert channel implementations
- Software-Defined Radio (SDR) integration
- Real-time signal processing and analysis
- Various modulation techniques (FSK, OOK)
- Testing and validation tools

## 📚 Documentation

Each directory contains its own detailed README with specific information about that component:

- For CS research implementations, check `/cs_research/readme.md`
- For receiver details and setup, see `/receiver/README.md`
- For transmitter specifications, refer to `/transmitter/README.md`

## 🛠️ Technologies Used

- **Programming Languages**: C, Python
- **Hardware**: HackRF, Raspberry Pi
- **Signal Processing**: FFT, Various Modulation Techniques
- **GUI**: Custom visualization tools
- **Testing**: Comprehensive test suites

## ⚠️ Disclaimer

This project is for research and educational purposes only. The implemented techniques should be used responsibly and in compliance with applicable laws and regulations.

## 📝 License

Please refer to the individual component directories for specific licensing information.

---

For detailed information about each component, please refer to the README files in their respective directories.
