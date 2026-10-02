#pragma once

#include "Uart.hpp"

#include <vector>

class Bus {
public:
    void attach(Uart& uart) {
        uarts_.push_back(&uart);
    }

    void tick() {
        for (Uart* uart : uarts_) {
            uart->tick();
        }
    }

private:
    std::vector<Uart*> uarts_;
};