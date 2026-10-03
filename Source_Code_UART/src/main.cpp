#include "../include/Bus.hpp"
#include "../include/Uart.hpp"

#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

static string readLineOrDefault(const string& prompt, const string& fallback) {
    cout << prompt << " [" << fallback << "]: ";
    string text;
    if (!getline(cin, text) || text.empty()) {
        return fallback;
    }
    return text;
}

static int readIntOrDefault(const string& prompt, int fallback, int minValue, int maxValue) {
    while (true) {
        cout << prompt << " [" << fallback << "]: ";
        string text;
        if (!getline(cin, text) || text.empty()) {
            return fallback;
        }

        stringstream stream(text);
        int value = 0;
        char extra = '\0';
        if (stream >> value && !(stream >> extra) && value >= minValue && value <= maxValue) {
            return value;
        }

        cout << "Please enter a number from " << minValue << " to " << maxValue << ".\n";
    }
}

static bool readYesNoOrDefault(const string& prompt, bool fallback) {
    const string fallbackText = fallback ? "y" : "n";
    while (true) {
        cout << prompt << " [" << fallbackText << "]: ";
        string text;
        if (!getline(cin, text) || text.empty()) {
            return fallback;
        }

        if (text == "y" || text == "Y" || text == "yes" || text == "YES") {
            return true;
        }
        if (text == "n" || text == "N" || text == "no" || text == "NO") {
            return false;
        }

        cout << "Please enter y or n.\n";
    }
}

static Uart::ParityMode parityFromChoice(int choice) {
    if (choice == 1) {
        return Uart::ParityMode::Even;
    }
    if (choice == 2) {
        return Uart::ParityMode::Odd;
    }
    return Uart::ParityMode::None;
}

static string parityName(Uart::ParityMode mode) {
    if (mode == Uart::ParityMode::Even) {
        return "even";
    }
    if (mode == Uart::ParityMode::Odd) {
        return "odd";
    }
    return "none";
}

static string toBinary8(uint32_t value) {
    string bits;
    bits.reserve(11U);
    for (int bit = 7; bit >= 0; --bit) {
        bits.push_back((value & (1U << bit)) != 0U ? '1' : '0');
        if (bit == 4) {
            bits.push_back(' ');
        }
    }
    return bits;
}

static void printStatusLine(const string& label, const Uart& uart) {
    const Uart::Registers registers = uart.registers();
    cout << label << ' ' << uart.name() << " STATUS[7..0]=" << toBinary8(registers.status)
         << " | b2=overflow:" << ((registers.status & Uart::StatusBufferOverflow) != 0U ? "1" : "0")
         << " b1=rx_ready:" << ((registers.status & Uart::StatusRxReady) != 0U ? "1" : "0")
         << " b0=tx_busy:" << ((registers.status & Uart::StatusTxBusy) != 0U ? "1" : "0") << '\n';
}

static string buildFrameBinary(const Uart::Config& config, std::uint8_t byte) {
    const int dataBits = config.dataBits == 7U ? 7 : 8;
    string frame = "0 ";

    for (int bit = 0; bit < dataBits; ++bit) {
        frame.push_back(((byte >> bit) & 1U) != 0U ? '1' : '0');
    }

    if (config.parityEnabled) {
        int ones = 0;
        for (int bit = 0; bit < dataBits; ++bit) {
            ones += (byte >> bit) & 1U;
        }
        const int parityValue = (config.parity == Uart::ParityMode::Even) ? (ones % 2) : ((ones % 2 == 0) ? 1 : 0);
        frame.push_back(' ');
        frame.push_back(parityValue != 0 ? '1' : '0');
    }

    frame.push_back(' ');
    for (std::uint8_t stop = 0U; stop < config.stopBits; ++stop) {
        frame.push_back('1');
    }

    return frame;
}

static int dataBitCount(const Uart::Config& config) {
    return config.dataBits == 7U ? 7 : 8;
}

static int calculateParityBit(const Uart::Config& config, std::uint8_t byte, int& onesCount) {
    onesCount = 0;
    const int bits = dataBitCount(config);
    for (int bit = 0; bit < bits; ++bit) {
        onesCount += (byte >> bit) & 1U;
    }

    if (!config.parityEnabled) {
        return -1;
    }

    if (config.parity == Uart::ParityMode::Even) {
        return onesCount % 2;
    }

    return (onesCount % 2 == 0) ? 1 : 0;
}

static void printConfigHeader(const Uart::Config& config) {
    cout << "================ UART CONFIGURATION ================\n";
    cout << "Baud rate : " << config.baudRate << '\n';
    cout << "Parity    : " << parityName(config.parity) << (config.parityEnabled ? " (enabled)" : " (disabled)") << '\n';
    cout << "Stop bits : " << static_cast<int>(config.stopBits) << '\n';
    cout << "Data bits : " << static_cast<int>(config.dataBits) << '\n';
    cout << "Loopback  : " << (config.loopback ? "enabled" : "disabled") << '\n';
    cout << "===================================================\n";
}

static void printPacketTrace(const Uart& uart, std::uint8_t byte, const string& prefix) {
    const Uart::Config config = uart.config();
    const int dataBits = config.dataBits == 7U ? 7 : 8;
    const std::uint8_t maskedByte = dataBits == 7 ? static_cast<std::uint8_t>(byte & 0x7FU) : byte;

    int ones = 0;
    const int parityBit = calculateParityBit(config, maskedByte, ones);

    cout << "\n-- " << prefix << " --\n";
    cout << "Character : '" << static_cast<char>(maskedByte) << "'\n";
    cout << "ASCII     : " << static_cast<int>(maskedByte) << '\n';
    cout << "Binary    : " << toBinary8(maskedByte) << '\n';
    cout << "Data bits : ";
    for (int bit = 0; bit < dataBits; ++bit) {
        cout << ((maskedByte >> bit) & 1U);
        if (bit + 1 < dataBits) {
            cout << ' ';
        }
    }
    cout << "  (LSB first)\n";

    if (config.parityEnabled) {
        cout << "Parity    : ones=" << ones << ", bit=" << parityBit << " (" << parityName(config.parity) << ")\n";
    } else {
        cout << "Parity    : disabled\n";
    }

    cout << "Frame     : " << buildFrameBinary(config, maskedByte) << '\n';
    cout << "            Start | Data | ";
    if (config.parityEnabled) {
        cout << "Parity | ";
    }
    cout << "Stop\n";
}

static void printRegisterReport(const Uart& uart) {
    const Uart::Registers registers = uart.registers();
    const Uart::Config config = uart.config();

    cout << "\n================ REGISTER REPORT ================\n";
    cout << "CONTROL  : 0x" << hex << registers.control << dec << "  [" << toBinary8(registers.control) << "]\n";
    cout << "  bit0 parity enable = " << ((registers.control & Uart::ControlParityEnable) != 0U ? "1" : "0") << '\n';
    cout << "  bit1 parity odd    = " << ((registers.control & Uart::ControlParityOdd) != 0U ? "1" : "0") << '\n';
    cout << "  bit2 stop bits     = " << ((registers.control & Uart::ControlTwoStopBits) != 0U ? "1" : "0") << '\n';
    cout << "  bit3 data bits     = " << ((registers.control & Uart::ControlSevenDataBits) != 0U ? "1" : "0") << '\n';
    cout << "  bit4 loopback      = " << ((registers.control & Uart::ControlLoopback) != 0U ? "1" : "0") << '\n';

    cout << "STATUS   : 0x" << hex << registers.status << dec << "  [" << toBinary8(registers.status) << "]\n";
    cout << "  bit0 TX_BUSY           = " << ((registers.status & Uart::StatusTxBusy) != 0U ? "1" : "0") << '\n';
    cout << "  bit1 RX_READY          = " << ((registers.status & Uart::StatusRxReady) != 0U ? "1" : "0") << '\n';
    cout << "  bit2 BUFFER_OVERFLOW    = " << ((registers.status & Uart::StatusBufferOverflow) != 0U ? "1" : "0") << '\n';

    cout << "BAUD_DIV : " << registers.baudDiv << '\n';
    cout << "  derived from base clock 16000000 / baud rate " << config.baudRate << '\n';
    cout << "================================================\n";
}

int main(int argc, char* argv[]) {
    string message = argc > 1 ? argv[1] : "Hello UART";

    cout << "UARTSim setup\n";
    cout << "Press Enter to accept each default.\n\n";

    message = readLineOrDefault("Message to send", message);
    const int parityChoice = readIntOrDefault("Parity 0=none 1=even 2=odd", 0, 0, 2);
    const int stopBitsChoice = readIntOrDefault("Stop bits 1 or 2", 1, 1, 2);
    const int dataBitsChoice = readIntOrDefault("Data bits 7 or 8", 8, 7, 8);
    const int baudRateChoice = readIntOrDefault("Baud rate", 9600, 300, 1000000);
    const bool loopbackChoice = readYesNoOrDefault("Loopback mode y/n", false);

    Uart uart0{"UART0"};
    Uart uart1{"UART1"};
    Bus bus;

    Uart::Config config;
    config.parity = parityFromChoice(parityChoice);
    config.parityEnabled = config.parity != Uart::ParityMode::None;
    config.stopBits = static_cast<std::uint8_t>(stopBitsChoice);
    config.dataBits = static_cast<std::uint8_t>(dataBitsChoice);
    config.baudRate = static_cast<std::uint32_t>(baudRateChoice);
    config.loopback = loopbackChoice;

    uart0.configure(config);
    uart1.configure(config);

    uart0.connectPeer(&uart1);
    uart1.connectPeer(&uart0);
    bus.attach(uart0);
    bus.attach(uart1);

    // ---- Register callbacks -------------------------------------------
    // These fire immediately from inside Uart::tick()/sendByte(), instead
    // of main() having to poll hasPendingRx()/status() to find out.

    uart0.setOnByteReceived([](std::uint8_t byte) {
        cout << "  [CALLBACK] UART0 received byte: '" << static_cast<char>(byte) << "' (0x" << hex
             << static_cast<int>(byte) << dec << ")\n";
    });
    uart1.setOnByteReceived([](std::uint8_t byte) {
        cout << "  [CALLBACK] UART1 received byte: '" << static_cast<char>(byte) << "' (0x" << hex
             << static_cast<int>(byte) << dec << ")\n";
    });

    uart0.setOnBufferOverflow([]() {
        cout << "  [CALLBACK] Buffer overflow on UART0!\n";
    });
    uart1.setOnBufferOverflow([]() {
        cout << "  [CALLBACK] Buffer overflow on UART1!\n";
    });

    uart0.setOnFramingError([]() {
        cout << "  [CALLBACK] Framing error detected on UART0!\n";
    });
    uart1.setOnFramingError([]() {
        cout << "  [CALLBACK] Framing error detected on UART1!\n";
    });

    Uart& receiver = config.loopback ? uart0 : uart1;
    const string senderName = uart0.name();
    const string receiverName = receiver.name();
    std::size_t framesTransmitted = 0U;
    std::size_t totalBitsSent = 0U;

    printConfigHeader(config);
    cout << "\n================ TRANSMISSION LOG ================\n";
    cout << "Message : " << message << '\n';
    cout << "Sender  : " << senderName << " -> Receiver: " << receiverName << '\n';
    cout << "==================================================\n";

    Uart::Registers previousTx = uart0.registers();
    Uart::Registers previousRx = uart1.registers();

    for (char ch : message) {
        const std::uint8_t byte = static_cast<std::uint8_t>(ch);
        const Uart::Config currentConfig = uart0.config();
        const int dataBits = dataBitCount(currentConfig);
        const std::uint8_t maskedByte = dataBits == 7 ? static_cast<std::uint8_t>(byte & 0x7FU) : byte;
        int ones = 0;
        const int parityBit = calculateParityBit(currentConfig, maskedByte, ones);

        printPacketTrace(uart0, byte, "TRANSMIT");
        cout << "  data bits   : " << toBinary8(maskedByte).substr(0, dataBits + (dataBits > 4 ? 1 : 0)) << " (LSB first)\n";
        cout << "  parity calc : ones=" << ones;
        if (currentConfig.parityEnabled) {
            cout << ", parity bit=" << parityBit;
        } else {
            cout << ", parity off";
        }
        cout << '\n';

        uart0.sendByte(byte);
        bus.tick();
        ++framesTransmitted;
        totalBitsSent += static_cast<std::size_t>(1 + dataBits + (currentConfig.parityEnabled ? 1 : 0) + currentConfig.stopBits);

        cout << "  frame       : Start | " << toBinary8(maskedByte).substr(0, dataBits + (dataBits > 4 ? 1 : 0));
        if (currentConfig.parityEnabled) {
            cout << " | " << parityBit;
        }
        cout << " | " << string(currentConfig.stopBits, '1') << '\n';

        Uart::Registers currentTx = uart0.registers();
        Uart::Registers currentRx = uart1.registers();
        if (currentTx.status != previousTx.status) {
            cout << "  STATUS TX changed: [" << toBinary8(currentTx.status) << "]";
            cout << " TX_BUSY=" << ((currentTx.status & Uart::StatusTxBusy) != 0U ? "1" : "0")
                 << " RX_READY=" << ((currentTx.status & Uart::StatusRxReady) != 0U ? "1" : "0")
                 << " OVERFLOW=" << ((currentTx.status & Uart::StatusBufferOverflow) != 0U ? "1" : "0") << '\n';
        }
        if (currentRx.status != previousRx.status) {
            cout << "  STATUS RX changed: [" << toBinary8(currentRx.status) << "]";
            cout << " TX_BUSY=" << ((currentRx.status & Uart::StatusTxBusy) != 0U ? "1" : "0")
                 << " RX_READY=" << ((currentRx.status & Uart::StatusRxReady) != 0U ? "1" : "0")
                 << " OVERFLOW=" << ((currentRx.status & Uart::StatusBufferOverflow) != 0U ? "1" : "0") << '\n';
        }
        previousTx = currentTx;
        previousRx = currentRx;
    }

    printStatusLine("End TX", uart0);
    printStatusLine("End RX", receiver);

    // Read the hardware buffer completely exactly ONCE into memory
    string receivedMessage = "";
    uint8_t rxByte = 0U;
    while (receiver.readReceivedByte(rxByte)) {
        receivedMessage.push_back(static_cast<char>(rxByte));
    }

    // Comprehensive Transmission and Reception Summary
    cout << "\n================ TRANSMISSION & RECEPTION SUMMARY ================\n";
    cout << "Sender Node      : " << senderName << '\n';
    cout << "Receiver Node    : " << receiverName << '\n';
    cout << "Characters Sent  : " << message.size() << '\n';
    cout << "Frames Sent      : " << framesTransmitted << '\n';
    cout << "Bits Per Frame   : " << (1 + dataBitCount(config) + (config.parityEnabled ? 1 : 0) + config.stopBits) << '\n';
    cout << "Total Bits Sent  : " << totalBitsSent << '\n';
    cout << "Chars Received   : " << receivedMessage.size() << '\n';
    cout << "Received Message : \"" << receivedMessage << "\"\n";
    cout << "==================================================================\n";

    printRegisterReport(uart0);
    return 0;
}