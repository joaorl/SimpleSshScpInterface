
#include "SshClient.h"

int main()
{
    string received;

    SshClient session("<ip-address>", "<user>", "<password>"); 
    session.Connect();
    /**
     * Example to execute a command in a remote machine and
     * read the returned data
     * 
     */
    session.Execute("ls -lha", &received);
    cout << received << endl;
    /**
     * Example to copy from a remote machine
     * 
     */
    session.Pull("/path/to/remote/file", "/path/to/local/file");
    /**
     * Example to copy to a remote machine
     * 
     */
    session.Push("/path/to/local/file", "/path/to/remote/file");

    session.Close();

    return 0;
}