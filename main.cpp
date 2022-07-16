
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