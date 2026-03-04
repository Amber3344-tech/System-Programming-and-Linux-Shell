#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include<sys/wait.h>
#include <stdlib.h>     /* exit, EXIT_FAILURE */
#include <iostream>
#include <vector>
#include "utils.cpp"
using namespace std;
int main()
{
    // simple shell implementation
    string line;
    vector<string> hisvec = gethistory("history.txt");

    while (true)
    {
        // print prompt
        cout << runCommand({"pwd"}) <<"> ";
        //cout.flush();

        // read a line from stdin using C++ getline
        if (!getline(cin, line)) {
            // EOF or error
            break;
        }

        // remove trailing carriage return if present (for Windows-style input)
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        // string comparison to check for "exit" command
        if (line == "exit") {
            break;
        }
        if (line.empty()) {
            continue;
        }

        // add the command to history
        hisvec.push_back(line);

        // tokenize the input into argv-style array
        const int MAXARGS = 64;
        char *argv[MAXARGS];
        int argc = 0;
        // strtok works on a mutable C string, so we need a backup buffer
        char buf[1024];
        strncpy(buf, line.c_str(), sizeof(buf)-1);
        buf[sizeof(buf)-1] = '\0';
        char *token = strtok(buf, " \t");
        while (token != NULL && argc < MAXARGS - 1) {
            argv[argc++] = token;
            token = strtok(NULL, " \t");
        }
        argv[argc] = NULL;

        // fork a child to execute the command
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            continue;
        }
        if (pid == 0) {
            // child process
            execvp(argv[0], argv);
            // if execvp returns, an error occurred
            perror("execvp");
            exit(EXIT_FAILURE);
        } else {
            // parent waits for child to finish
            int status;
            waitpid(pid, &status, 0);
        }
    }

    cout << "Exiting shell.\n";
    return 0;

    }
