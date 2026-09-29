# UARTSim: UART Peripheral Simulator in C++

UARTSim is a high-fidelity software simulation of a **Universal Asynchronous Receiver/Transmitter (UART)** peripheral. Built as a comprehensive demonstration of hardware-software co-design principles, this simulation models the low-level register behaviors, buffering strategies, and interrupt-driven concepts common in embedded systems.

## Key Features

*   **Memory-Mapped Registers:** Simulates hardware register spaces allowing realistic software-to-peripheral register interactions.
*   **Loopback & Link Demonstration:** Integrates two interconnected UART instances to actively simulate the physical transmission of serial data from a transmitter (TX) to a receiver (RX).
*   **Circular Buffers:** Implements dedicated, robust FIFO queues for both transmit and receive paths to manage data streams asynchronously.
*   **Object-Oriented Design:** Leverages C++ polymorphism to establish a scalable, clean peripheral hierarchy.
*   **Modern C++ Callbacks:** Employs C++ lambda functions to construct a clean, modern event-driven callback system mimicking hardware interrupts.

## Concepts Demonstrated

This project serves as a final capstone for the **C/C++ for Hardware Engineers** module, effectively bridging pure software architecture with hardware-level engineering mechanics:
- Hardware-abstracted object modeling
- Memory safety and resource constraints in communication peripherals
- Event notification systems within strict hardware constraints

## Tech Stack
- **Language:** C++
- **Paradigm:** Object-Oriented Programming (OOP) & Event-Driven Design