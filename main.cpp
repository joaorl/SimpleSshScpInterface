
#include "SshClient.h"

int main()
{
    string received;

    SshClient session("localhost", "jlima", "qwertqwert"); 
    session.Demo();
    return 0;
}