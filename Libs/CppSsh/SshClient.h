
#ifndef __SSH_CLIENT_H__
#define __SSH_CLIENT_H__

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <errno.h>
#include <iostream>
#include <vector>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

using namespace std;

class SshClient
{
public:
    SshClient() = delete;
    SshClient(string ip, string user, string password):
              _ip(ip), _user(user), _password(password){};
    SshClient(string ip, string user, string password, bool autoverifyhost):
              _ip(ip), _user(user), _password(password), _autoverifyhost(autoverifyhost){};

    int Demo()
    {
        // create an empty structure (null)
        json j;

        // add a number that is stored as double (note the implicit conversion of j to an object)
        j["pi"] = 3.141;


        cout << j << endl;
        return 0;
    };
private:
    string _ip, _user, _password;
    bool _autoverifyhost{true};
};

#endif // __SSH_CLIENT_H__