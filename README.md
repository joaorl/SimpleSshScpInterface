# SSH Client
This is a C++ library for executing commands and transferring files through SSH. It provides a simple and easy-to-use interface for connecting to a remote host, authenticating with a user and password, and executing commands or transferring files.

## Features
 * Connect to a remote host using an IP address, user name, and password
 * Authenticate using either console-based or keyboard-interactive methods
 * Verify the host's identity using its public host key
 * Execute commands on the remote host and retrieve the output
 * Transfer files to and from the remote host using scp

## Dependencies
This library depends on the following libraries:

* `libssh`: a cross-platform C library implementing the SSHv2 protocol
* `std`: the C++ standard library

# Usage
To use this library, you need to include the SshClient.h header file and create an instance of the SshClient class with the IP address, user name, and password of the remote host. Then, you can call the various methods of the class to execute commands and transfer files.
```
#include "SshClient.h"

int main()
{
    // Connect to the remote host
    SshClient session("192.168.0.1", "user", "password");
    session.Connect();

    // Execute a command and retrieve the output
    string output;
    session.Execute("ls -l", &output);
    cout << output << endl;

    // Transfer a file from the local host to the remote host
    session.Push("local/path/to/file.txt", "remote/path/to/file.txt");

    // Transfer a file from the remote host to the local host
    session.Pull("remote/path/to/file.txt", "local/path/to/file.txt");

    // Disconnect from the remote host
    session.Close();

    return 0;
}
```
## Connect
```
int Connect();
```
Establishes a connection to the remote host and authenticates the user.

Returns 0 on success, or a negative value on error.

## Execute
```
int Execute(string command, bool verbosity);
int Execute(string command, string* received);
int Execute(string command, string* received, bool verbosity);
```
Executes a command on the remote host and retrieves the output.
The first version of this method executes the command and prints the output to the console if verbosity is true.
The second version stores the output in the string pointed to by received.
The third version allows you to specify whether to print the output to the console or not.

Returns 0 on success, or a negative value on error.

## Push
```
int Push(string source, string destination);
```
Transfers a file or directory from the local host to the remote host using scp.
The source parameter specifies the path to the file or directory on the local host.
The destination parameter specifies the path to the destination on the remote host.

Returns 0 on success, or a negative value on error.

## Pull
```
int Pull(string source, string destination);
```
Transfers a file or directory from the remote host to the local host using `scp`.
The `source` parameter specifies the path to the file or directory on the remote host.
The `destination` parameter specifies the path to the destination on the local host.

Returns 0 on success, or a negative value on error.

## Close
```
void Close();
```

Disconnects from the remote host and frees the resources used by the SSH session.