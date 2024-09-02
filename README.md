# Testing Server

## Overview

The Testing Server project is designed to create a client-server architecture that allows clients to send code files to a server for compilation and execution. This system is particularly useful for environments where code needs to be tested remotely or in an automated manner. The server receives the code, compiles it, and runs the executable, returning the results to the client.

## Features

- **Client-Server Architecture:** Clients can connect to the server, send code files, and receive execution results.
- **Remote Compilation:** The server compiles the received code files using `gcc` and runs the generated executables.
- **Packet-based Communication:** The system uses a custom packet structure for communication between the client and server, ensuring reliable and organized data transmission.
- **Multi-threading:** The server handles multiple clients simultaneously using multi-threading, allowing for concurrent processing of client requests.
- **Error Handling:** Comprehensive error handling ensures that issues such as failed compilation or runtime errors are communicated back to the client.

## Files

- `client.c`: Contains the client-side code for connecting to the server, sending code files, and receiving results.
- `testServer.c`: Contains the server-side code that listens for client connections, compiles the received code files, and runs the executables.
- `packet.h`: Defines the structure and functions for the custom packet used for communication between the client and server.
- `connInfo.h`: Defines the structure for storing connection information on the client side.

## How It Works

1. **Client Connection:**
   - The client connects to the server using a specified IP address and port.
   - After establishing a connection, the client enters a shell where it can send commands to the server.

2. **Sending Files:**
   - The client can send code files to the server using a specific command (e.g., `test <filename> <testname>`).
   - The server receives the file, compiles it, and runs the executable.

3. **Receiving Results:**
   - The server sends the results of the compilation and execution back to the client.
   - If the execution is successful, the client receives the output of the program. If there are errors, the client is notified accordingly.

### Prerequisites

- **GCC Compiler:** Ensure `gcc` is installed on the server to compile the code files.
- **POSIX Threads:** The server uses pthreads for multi-threading, so a POSIX-compliant system is required.

### Building the Project

1. Clone the repository to both client and server machines:
   ```bash
   git clone https://github.com/boezienko/testbedserver.git
   cd testbedserver
   ```
2. Compile the server and client on their respective machines:
   ```bash
   gcc testServer.c -o server
   gcc client.c -o client
   ```
3. Run the server:
   ```bash
   ./server
   ```
4. Run the client:
   ```bash
   ./client <server-ip-address>
   ```

### Usage
- **Client Commands**
   - `test <filename> <testname>`: Sends the specified file to the server for testing with the given test name.
   - `ls-tests`: Lists available test files on the server.
   - `quit`: Disconnects from the server.
 - **Server Actions**
   - Receives and compiles the code file.
   - Executes compiled program and and sends results back to client.
   - Accepts and processes commands from the client (`ls-tests`, `quit`).
