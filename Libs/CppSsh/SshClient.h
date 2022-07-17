
#ifndef __SSH_CLIENT_H__
#define __SSH_CLIENT_H__

#include <libssh/libssh.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <errno.h>
#include <iostream>
#include <vector>

using namespace std;

class SshClient
{
public:
    SshClient() = delete;
    SshClient(string ip, string user, string password): _ip(ip), _user(user),
                                                        _password(password){};
    int Connect();
    int Execute(string command, bool verbosity);
    int Execute(string command, string* received);
    int Execute(string command, string* received, bool verbosity);
    int Push(string source, string destination);
    int Pull(string source, string destination);
    void Close();

private:
    int _AuthenticateConsole(ssh_session& session);
    int _AuthenticateKbdint(ssh_session& session, const char *password);
    int _VerifyKnownhost(ssh_session& session);
    ssh_session _Connect(const char *hostname, const char *user,
                         const char *password, int verbosity);
    int _CopyToRemote(ssh_session& session, string source, string destination);
    int _CopyFromRemote(ssh_session& session, string source, string destination);
    int _CreateRemoteFolder(ssh_session& session, ssh_scp& scp, string name);
    int _CreateRemoteFile(ssh_session& session, ssh_scp& scp, string destination,
                          const char *buffer, size_t length);
    int _CreateRemoteFilesTree(ssh_session& session, ssh_scp& scp,
                               string source, string destination);
    int _CreateLocalFilesTree(ssh_session& session, ssh_scp& scp,
                              string destination);
private:
    string _ip, _user, _password;
    ssh_session _session;
    ssh_channel _channel;
};

#endif // __SSH_CLIENT_H__