# TestbedServer

## testbedServer.c
This file is the actual server script that listens for connections, then creates a thread and socket for each connection for communication with a client using a tcp stream socket.

## client.c
This file is for clients to run to connect with the server to test their programs.<br> It should be compiled like this:gcc client.c -o <outputFileName>. Then run the program like this: ./<outputFileName> <fileToTest> <testcaseName>. Ensure that the fileToTest is in the same directory as client.c
