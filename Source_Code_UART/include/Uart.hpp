#pragma once

#include "CircularBuffer.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <utility>
using namespace std;

class Uart {
public:
    enum class ParityMode {
        None,
        Even,
        Odd,
    };

    struct Config {
        ParityMode parity = ParityMode::None;
        bool parityEnabled = false;
        uint8_t stopBits = 1U;
        uint8_t dataBits = 8U;
        uint32_t baudRate = 9600U;
        bool loopback = false;
    };

    struct Registers {
        uint32_t control = 0U;
        uint32_t baudDiv = 1U;
        uint32_t status = 0U;
    };

    enum ControlBit : std::uint32_t {
        ControlParityEnable = 1U << 0,
        ControlParityOdd = 1U << 1,
        ControlTwoStopBits = 1U << 2,
        ControlSevenDataBits = 1U << 3,
        ControlLoopback = 1U << 4,
    };

    enum StatusBit : std::uint32_t {
        StatusTxBusy = 1U << 0,
        StatusRxReady = 1U << 1,
        StatusBufferOverflow = 1U << 2,
    };

    explicit Uart(std::string name)
        : name_(std::move(name)) {
    }

    // ---- Callback registration -------------------------------------------
  

    void setOnByteReceived(std::function<void(std::uint8_t)> callback) {
        onByteReceived_ = std::move(callback);
    }

    void setOnBufferOverflow(std::function<void()> callback) {
        onBufferOverflow_ = std::move(callback);
    }

    void setOnFramingError(std::function<void()> callback) {
        onFramingError_ = std::move(callback);
    }

    void connectPeer(Uart* peer) {
        peer_ = peer;
    }

    void configure(const Config& config) {
        config_ = config;
        if (!config_.parityEnabled) {
            config_.parity = ParityMode::None;
        }

        registers_.control = 0U;
        if (config_.parityEnabled) {
            registers_.control |= ControlParityEnable;
            if (config_.parity == ParityMode::Odd) {
                registers_.control |= ControlParityOdd;
            }
        }
        if (config_.stopBits == 2U) {
            registers_.control |= ControlTwoStopBits;
        }
        if (config_.dataBits == 7U) {
            registers_.control |= ControlSevenDataBits;
        }
        if (config_.loopback) {
            registers_.control |= ControlLoopback;
        }

        registers_.baudDiv = config_.baudRate == 0U ? 1U : (BaseClockHz / config_.baudRate);
        if (registers_.baudDiv == 0U) {
            registers_.baudDiv = 1U;
        }

        registers_.status = 0U;
    }

    void send(const std::string& text) {
        for (char ch : text) {
            sendByte(static_cast<std::uint8_t>(ch));
        }

        if (!txBuffer_.empty()) {
            registers_.status |= StatusTxBusy;
        }
    }

    void sendByte(std::uint8_t byte) {
        if (!txBuffer_.push(byte)) {
            registers_.status |= StatusBufferOverflow;
            if (onBufferOverflow_) {
                onBufferOverflow_();
            }
            return;
        }

        registers_.status |= StatusTxBusy;
    }

    void tick() {
        uint8_t byte = 0U;
        if (!txBuffer_.pop(byte)) {
            if (txBuffer_.empty()) {
                registers_.status &= static_cast<std::uint32_t>(~StatusTxBusy);
            }
            return;
        }

        if (config_.loopback) {
            if (rxBuffer_.push(byte)) {
                registers_.status |= StatusRxReady;
                if (onByteReceived_) {
                    onByteReceived_(byte);
                }
            } else {
                registers_.status |= StatusBufferOverflow;
                if (onBufferOverflow_) {
                    onBufferOverflow_();
                }
            }
        } else if (peer_ != nullptr) {
            // A framing error is modeled as a frame-format mismatch between
            // sender and receiver (baud rate / data bits / stop bits) -
            // this is the real-world condition that causes a UART to
            // misread the stop bit and flag a framing error.
            if (config_.baudRate != peer_->config_.baudRate ||
                config_.dataBits != peer_->config_.dataBits ||
                config_.stopBits != peer_->config_.stopBits) {
                if (peer_->onFramingError_) {
                    peer_->onFramingError_();
                }
            }

            if (peer_->rxBuffer_.push(byte)) {
                peer_->registers_.status |= StatusRxReady;
                if (peer_->onByteReceived_) {
                    peer_->onByteReceived_(byte);
                }
            } else {
                peer_->registers_.status |= StatusBufferOverflow;
                if (peer_->onBufferOverflow_) {
                    peer_->onBufferOverflow_();
                }
            }
        }

        if (txBuffer_.empty()) {
            registers_.status &= static_cast<std::uint32_t>(~StatusTxBusy);
        }
    }

    bool readReceivedByte(uint8_t& byte) {
        const bool popped = rxBuffer_.pop(byte);
        if (rxBuffer_.empty()) {
            registers_.status &= static_cast<uint32_t>(~StatusRxReady);
        }
        return popped;
    }

    bool hasPendingTx() const {
        return !txBuffer_.empty();
    }

    bool hasPendingRx() const {
        return !rxBuffer_.empty();
    }

    const string& name() const {
        return name_;
    }

    const Config& config() const {
        return config_;
    }

    const Registers& registers() const {
        return registers_;
    }

private:
    static constexpr std::uint32_t BaseClockHz = 16000000U;

    string name_;
    Uart* peer_ = nullptr;
    Config config_{};
    Registers registers_{};
    CircularBuffer<std::uint8_t, 16U> txBuffer_;
    CircularBuffer<std::uint8_t, 16U> rxBuffer_;

    std::function<void(std::uint8_t)> onByteReceived_;
    std::function<void()> onBufferOverflow_;
    std::function<void()> onFramingError_;
};