# EmbedKit

## UART Frame Parser

Implementation of a UART frame parser using a Finite State Machine (FSM) in C. The parser processes incoming bytes one at a time, validates frames using an XOR checksum, supports inter-byte timeout detection, and automatically recovers from corrupted or incomplete frames.

## Features

* Byte-by-byte UART frame parsing
* Finite State Machine (FSM) based design
* XOR checksum verification
* Inter-byte timeout detection
* Automatic parser reset and recovery
* Configurable timeout value
* Supports payload lengths from 0 to 16 bytes
* Demonstrates all required test cases from the assignment


## Protocol Format

text
+------+------+---------+-------------+----------+
| SOF  | CMD  | LEN     | PAYLOAD     | CHECKSUM |
+------+------+---------+-------------+----------+
| 0xAA | 1 B  | 1 B     | 0-16 Bytes  | 1 B      |
+------+------+---------+-------------+----------+


Checksum:

text
CHECKSUM = CMD ^ LEN ^ PAYLOAD[0] ^ ... ^ PAYLOAD[N-1]


## Files

| File          | Description                                               |
| ------------- | --------------------------------------------------------- |
| uart_parser.c | Complete UART frame parser implementation with test cases |
| README.md     | Project documentation and build instructions              |



## Build

Compile using GCC:

bash
gcc -Wall -std=c99 uart_parser.c -o uart_parser


The code compiles with:

* Zero errors
* Zero warnings



## Run

bash
./uart_parser


## Test Cases Implemented

### Test 1

Valid frame parsing.

### Test 2

Timeout during frame reception followed by parser recovery.

### Test 3

Two valid frames received back-to-back.

### Test 4

Timeout disabled, resulting in checksum failure instead of timeout reset.

## Expected Return Codes

| Return Value | Meaning              |
| ------------ | -------------------- |
| 1            | Valid frame received |
| 0            | Frame in progress    |
| -1           | Checksum mismatch    |
| -2           | Inter-byte timeout   |


## Author

Manoj Kumar


