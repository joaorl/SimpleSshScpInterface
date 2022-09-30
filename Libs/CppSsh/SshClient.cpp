
#include "SshClient.h"

#include <sys/stat.h>
#include <filesystem>
#include <fcntl.h>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

static void error(ssh_session session)
{
    printf("Authentication failed: %s\n", ssh_get_error(session));
}

vector<string> split(const string& str, const string& delimiter)
{
    vector<string> substrings;
    string::size_type pos = 0;
    string::size_type prev = 0;

    while ((pos = str.find(delimiter, prev)) != string::npos)
    {
        substrings.push_back(str.substr(prev, pos - prev));
        prev = pos + 1;
    }

    substrings.push_back(str.substr(prev));

    return substrings;
}

int SshClient::Connect()
{
    _session = _Connect(_ip.c_str(), _user.c_str(), _password.c_str(), 0);
    if (_session == NULL)
    {
        return SSH_ERROR;
    }

    _channel = ssh_channel_new(_session);
    if (_channel == NULL)
    {
        ssh_disconnect(_session);

        return SSH_ERROR;
    }

    int res = ssh_channel_open_session(_channel);
    if (res < 0)
    {
        ssh_channel_close(_channel);
        ssh_disconnect(_session);

        return SSH_ERROR;
    }

    return SSH_OK;
};

int SshClient::Push(string source, string destination)
{
    return _CopyToRemote(_session, source, destination);
}

int SshClient::Pull(string source, string destination)
{
    return _CopyFromRemote(_session, source, destination);
}

int SshClient::_CreateRemoteFilesTree(ssh_session& session, ssh_scp& scp,
                                      string source, string destination)
{
    int res = SSH_OK;

    if (fs::is_directory(source) == false)
    {
        printf("INFO - Uploading file %s, permissions 0\n",source.c_str());
        ifstream input(source, std::ios::binary);
        constexpr size_t bufferSize = 1024 * 1024;
        unique_ptr<char[]> buffer(new char[bufferSize]);

        while (!input.eof())
        {
            input.read(buffer.get(), bufferSize);
            _CreateRemoteFile(session, scp, destination, buffer.get(),
                              strlen(buffer.get()));
        }

        return 0;
    }

    printf("INFO - Uploading directory %s, permissions 0\n", source.c_str());
    // cout << "Destination = " << destination << endl;
    fs::current_path(source);

    res = _CreateRemoteFolder(session, scp, destination);
    if (res != SSH_OK)
    {
        fprintf(stderr, "Error to create folder: %s\n",
                ssh_get_error(session));
        ssh_scp_free(scp);

        return res;
    }

    try
    {
        for (fs::directory_iterator s("."), end; s != end; ++s)
        {
            _CreateRemoteFilesTree(session, scp, s->path().filename(),
                                   destination + "/" + s->path().filename().c_str());
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    ssh_scp_leave_directory(scp);

    fs::current_path("../");

    return 0;
}

int SshClient::_CopyToRemote(ssh_session& session, string source, string destination)
{
    ssh_scp scp;
    int res;

    scp = ssh_scp_new(session, SSH_SCP_WRITE | SSH_SCP_RECURSIVE, ".");
    if (scp == NULL)
    {
        fprintf(stderr, "Error allocating scp session: %s\n",
                ssh_get_error(session));
        return SSH_ERROR;
    }

    res = ssh_scp_init(scp);
    if (res != SSH_OK)
    {
        fprintf(stderr, "Error initializing scp session: %s\n",
                ssh_get_error(session));
        ssh_scp_free(scp);
        return res;
    }

    _CreateRemoteFilesTree(session, scp, source, destination);

    ssh_scp_close(scp);
    ssh_scp_free(scp);

    return SSH_OK;
}

int SshClient::_CopyFromRemote(ssh_session& session, string source,
                               string destination)
{
    ssh_scp scp;
    int res;

    scp = ssh_scp_new(session, SSH_SCP_READ | SSH_SCP_RECURSIVE, source.c_str());
    if (scp == NULL)
    {
        fprintf(stderr, "Error allocating scp session: %s\n",
                ssh_get_error(session));
        return SSH_ERROR;
    }

    res = ssh_scp_init(scp);
    if (res != SSH_OK)
    {
        fprintf(stderr, "Error initializing scp session: %s\n",
                ssh_get_error(session));
        ssh_scp_free(scp);
        return res;
    }

    _CreateLocalFilesTree(session, scp, destination);

    ssh_scp_close(scp);
    ssh_scp_free(scp);

    return SSH_OK;
}

int SshClient::_CreateLocalFilesTree(ssh_session& session, ssh_scp& scp,
                                     string destination)
{
    char *filename, *buffer;
    int res, size, mode;

    fs::current_path(destination);

    do
    {
        res = ssh_scp_pull_request(scp);
        // cout << "State = " << res << endl;
        switch(res)
        {
        case SSH_SCP_REQUEST_NEWFILE:
        {
            size = ssh_scp_request_get_size(scp);
            filename = strdup(ssh_scp_request_get_filename(scp));
            mode = ssh_scp_request_get_permissions(scp);
            printf("INFO - Receiving file %s, size %d, permissions 0%o\n",
                   filename, size, mode);

            ofstream localfile;
            localfile.open(destination);

            int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC);
            free(filename);

            buffer = (char*) malloc(size);
            if (buffer == NULL)
            {
                fprintf(stderr, "Memory allocation error\n");
                localfile.close();
                close(fd); // todo decide what is the best approach to save file
                return SSH_ERROR;
            }

            ssh_scp_accept_request(scp);
            res = ssh_scp_read(scp, buffer, size);
            if (res == SSH_ERROR)
            {
                fprintf(stderr, "Error receiving file data: %s\n",
                        ssh_get_error(session));
                free(buffer);
                localfile.close();
                close(fd); // todo decide what is the best approach to save file
                return res;
            }

            localfile << buffer;
        
            write(fd, buffer, size);
            localfile.close();
            close(fd); // todo decide what is the best approach to save file
            free(buffer);

            break;
        }
        case SSH_SCP_REQUEST_NEWDIR:
            filename = strdup(ssh_scp_request_get_filename(scp));
            mode = ssh_scp_request_get_permissions(scp);

            printf("INFO - Downloading directory %s, permissions 0%o\n",filename, mode);

            fs::create_directory(filename);
            fs::current_path(filename);

            free(filename);
            ssh_scp_accept_request(scp);

            break;

        case SSH_SCP_REQUEST_WARNING:
            fprintf(stderr, "Warning: %s\n",ssh_scp_request_get_warning(scp));
            break;

        case SSH_SCP_REQUEST_ENDDIR:
            // printf("End of directory\n");
            fs::current_path("..");

            break;

        case SSH_SCP_REQUEST_EOF:
            fprintf(stderr, "Unexpected request: %s\n",
                    ssh_get_error(session));

            return SSH_OK;

        case SSH_ERROR:
            fprintf(stderr, "Error: %s\n", ssh_get_error(session));

            return SSH_ERROR;
        }
    } while(1);

    return SSH_OK;
}

int SshClient::_CreateRemoteFolder(ssh_session& session, ssh_scp& scp, string name)
{
    int res;

    res = ssh_scp_push_directory(scp, name.c_str(), S_IRWXU);
    if (res != SSH_OK)
    {
        fprintf(stderr, "Can't create remote directory: %s\n",
                ssh_get_error(session));
        return res;
    }

    return SSH_OK;
}

int SshClient::_CreateRemoteFile(ssh_session& session, ssh_scp& scp,
                                 string destination, const char *buffer,
                                 size_t length)
{
    int res;


    res = ssh_scp_push_file(scp, destination.c_str(), length, S_IRWXU);
    if (res != SSH_OK)
    {
        fprintf(stderr, "Can't open remote file: %s\n",
                ssh_get_error(session));
        return res;
    }

    res = ssh_scp_write(scp, buffer, length);
    if (res != SSH_OK)
    {
        fprintf(stderr, "Can't write to remote file: %s\n",
                ssh_get_error(session));
        return res;
    }

  return SSH_OK;
}

int SshClient::Execute(string command, string* received, bool verbosity)
{
    char buffer[256] = {};
    string receive = "";
    size_t nbytes;
    int res;

    res = ssh_channel_request_exec(_channel, command.c_str());
    if (res < 0)
    {
        ssh_channel_close(_channel);
        ssh_disconnect(_session);

        return SSH_ERROR;
    }

    nbytes = ssh_channel_read(_channel, buffer, sizeof(buffer), 0);
    while (nbytes > 0)
    {
        try
        {
            string line(buffer, nbytes);
            if (verbosity == true)
            {
                cout << line << endl;
            }
            if (received)
            {
                (*received).append(line);
            }
        }
        catch (const std::exception& e)
        {
            break;
        }

        nbytes = ssh_channel_read(_channel, buffer, sizeof(buffer), 0);
    }

    if (nbytes < 0)
    {
        ssh_channel_close(_channel);
        ssh_channel_free(_channel);

        return SSH_ERROR;
    }

    return SSH_OK;
};

int SshClient::Execute(string command, bool verbosity)
{ 
    return Execute(command, NULL, verbosity);
}

int SshClient::Execute(string command, string* received)
{
    return Execute(command, received, false);
}
    
void SshClient::Close()
{
    if (_channel)
    {
        ssh_channel_send_eof(_channel);
        ssh_channel_close(_channel);
        ssh_channel_free(_channel);
    }
    if (_session)
    {
        ssh_disconnect(_session);
    }
}

ssh_session SshClient::_Connect(const char *host, const char *user,
                                const char *password, int verbosity)
{
    int auth = 0;
    int res;

    _session = ssh_new();
    if (_session == NULL)
    {
        return NULL;
    }

    if (user != NULL)
    {
        res = ssh_options_set(_session, SSH_OPTIONS_USER, user);
        if (res < 0)
        {
            ssh_disconnect(_session);

            return NULL;
        }
    }

    res = ssh_options_set(_session, SSH_OPTIONS_HOST, host);
    if (res < 0)
    {
        return NULL;
    }

    ssh_options_set(_session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);

    res = ssh_connect(_session);
    if (res != 0)
    {
        printf("Connection failed : %s\n",ssh_get_error(_session));

        ssh_disconnect(_session);

        return NULL;
    }

    res = _VerifyKnownhost(_session);
    if (res < 0)
    {
        ssh_disconnect(_session);

        return NULL;
    }

    if (password)
    {
        res = ssh_userauth_password(_session, NULL, password);
        if (res == SSH_AUTH_ERROR)
        {
            error(_session);

            return NULL;
        }
        else if (res == SSH_AUTH_SUCCESS)
        {
            char *banner;

            banner = ssh_get_issue_banner(_session);
            if (banner)
            {
                printf("%s\n",banner);
                free(banner);
            }

            return _session;
        }
    }

    auth = _AuthenticateConsole(_session);
    if (auth == SSH_AUTH_SUCCESS)
    {
        return _session;
    }
    else if(auth == SSH_AUTH_DENIED)
    {
        printf("Authentication failed\n");
    }
    else
    {
        printf("Error while authenticating : %s\n", ssh_get_error(_session));
    }

    ssh_disconnect(_session);

    return NULL;
}

int SshClient::_AuthenticateKbdint(ssh_session& session, const char *password)
{
    int err;

    err = ssh_userauth_kbdint(session, NULL, NULL);
    while (err == SSH_AUTH_INFO)
    {
        const char *instruction;
        const char *name;
        char buffer[128];
        int i, n;
        int res;

        name = ssh_userauth_kbdint_getname(session);
        instruction = ssh_userauth_kbdint_getinstruction(session);
        n = ssh_userauth_kbdint_getnprompts(session);

        if (name && strlen(name) > 0)
        {
            printf("%s\n", name);
        }

        if (instruction && strlen(instruction) > 0)
        {
            printf("%s\n", instruction);
        }

        for (i = 0; i < n; i++)
        {
            const char *answer;
            const char *prompt;
            char echo;

            prompt = ssh_userauth_kbdint_getprompt(session, i, &echo);
            if (prompt == NULL)
            {
                break;
            }

            if (echo)
            {
                char *p;

                printf("%s", prompt);

                if (fgets(buffer, sizeof(buffer), stdin) == NULL)
                {
                    return SSH_AUTH_ERROR;
                }

                buffer[sizeof(buffer) - 1] = '\0';
                if ((p = strchr(buffer, '\n')))
                {
                    *p = '\0';
                }

                res = ssh_userauth_kbdint_setanswer(session, i, buffer);
                if (res < 0)
                {
                    return SSH_AUTH_ERROR;
                }

                memset(buffer, 0, strlen(buffer));
            }
            else
            {
                if (password && strstr(prompt, "Password:"))
                {
                    answer = password;
                }
                else
                {
                    buffer[0] = '\0';

                    res = ssh_getpass(prompt, buffer, sizeof(buffer), 0, 0);
                    if (res < 0)
                    {
                        return SSH_AUTH_ERROR;
                    }
                    answer = buffer;
                }

                res = ssh_userauth_kbdint_setanswer(session, i, answer);
                if (res < 0)
                {
                    return SSH_AUTH_ERROR;
                }
            }
        }
        err = ssh_userauth_kbdint(session, NULL, NULL);
    }

    return err;
}

int SshClient::_AuthenticateConsole(ssh_session& session)
{
    char password[128] = {0};
    char *banner;
    int method;
    int res;

    res = ssh_userauth_none(session, NULL);
    if (res == SSH_AUTH_ERROR)
    {
        error(session);

        return res;
    }

    method = ssh_auth_list(session);
    while (res != SSH_AUTH_SUCCESS)
    {
        if (method & SSH_AUTH_METHOD_PUBLICKEY)
        {
            res = ssh_userauth_autopubkey(session, NULL);
            if (res == SSH_AUTH_ERROR)
            {
                error(session);

                return res;
            }
            else if (res == SSH_AUTH_SUCCESS)
            {
                break;
            }
        }

        if (method & SSH_AUTH_METHOD_INTERACTIVE)
        {
            res = _AuthenticateKbdint(session, password);
            if (res == SSH_AUTH_ERROR)
            {
                error(session);

                return res;
            }
            else if (res == SSH_AUTH_SUCCESS)
            {
                break;
            }
        }

        res = ssh_getpass("Password: ", password, sizeof(password), 0, 0);
        if (res < 0)
        {
            return SSH_AUTH_ERROR;
        }

        if (method & SSH_AUTH_METHOD_PASSWORD)
        {
            res = ssh_userauth_password(session, NULL, password);
            if (res == SSH_AUTH_ERROR)
            {
                error(session);

                return res;
            }
            else if (res == SSH_AUTH_SUCCESS)
            {
                break;
            }
        }
    }

    banner = ssh_get_issue_banner(session);
    if (banner)
    {
        printf("%s\n",banner);
        free(banner);
    }

    return res;
}

int SshClient::_VerifyKnownhost(ssh_session& session)
{
    char buf[10];
    int state;
    int res;

    state = ssh_session_is_known_server(session);

    switch(state)
    {
    case SSH_SERVER_KNOWN_OK:
        break; /* ok */
    case SSH_SERVER_KNOWN_CHANGED:
    {
        printf("Host key for server changed : server's one is now :\n");
        printf("For security reason, connection will be stopped\n");
        return -1;
    }
    case SSH_SERVER_FOUND_OTHER:
    {
        printf("The host key for this server was not found but an other type of key exists.\n");
        printf("An attacker might change the default server key to confuse your client"
            "into thinking the key does not exist\n"
            "We advise you to rerun the client with -d or -r for more safety.\n");
        return -1;
    }
    case SSH_SERVER_FILE_NOT_FOUND:
        printf("Could not find known host file. If you accept the host key here,\n");
        printf("the file will be automatically created.\n");
        /* fallback to SSH_SERVER_NOT_KNOWN behavior */
    case SSH_SERVER_NOT_KNOWN:
    {
        if (_autoverifyhost)
        {
            break;
        }


        printf("The server is unknown. Do you trust the host key ? (yes|no)\n");
        if (fgets(buf, sizeof(buf), stdin) == NULL)
        {
            return -1;
        }
        res = strncasecmp(buf, "yes", 3);
        if (res != 0)
        {
            return -1;
        }

        printf("This new key will be written on disk for further usage. do you agree ? (yes|no)\n");
        if (fgets(buf, sizeof(buf), stdin) == NULL)
        {
            return -1;
        }
        res = strncasecmp(buf, "yes", 3);
        if (res == 0)
        {
            res = ssh_session_update_known_hosts(session);
            if (res < 0)
            {
                printf("error %s\n", strerror(errno));
                return -1;
            }
        }

        break;
    }
    case SSH_SERVER_ERROR:
    {
        printf("%s",ssh_get_error(session));
        return -1;
    }
    }

    return 0;
}
