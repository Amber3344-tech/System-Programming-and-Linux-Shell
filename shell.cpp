#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <fcntl.h>
#include "utils.cpp"

using namespace std;

int main()
{
    string line;
    vector<string> hisvec = gethistory("history.txt");

    while (true)
    {
        cout << runCommand({"pwd"}) << "> ";

        if (!getline(cin, line))
            break;

        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        if (line == "exit")
            break;

        if (line.empty())
            continue;

        if (line == "history") {
            for (size_t i = 0; i < hisvec.size(); i++) {
                cout << i + 1 << " " << hisvec[i] << endl;
            }
            continue;
        }

        // add to history
        hisvec.push_back(line);
        ofstream outfile("history.txt");
        for (const auto& cmd : hisvec)
            outfile << cmd << endl;

        // tokenize
        const int MAXARGS = 64;
        char *argv[MAXARGS];
        int argc = 0;

        char buf[1024];
        strncpy(buf, line.c_str(), sizeof(buf)-1);
        buf[sizeof(buf)-1] = '\0';

        char *token = strtok(buf, " \t");
        while (token != NULL && argc < MAXARGS - 1) {
            argv[argc++] = token;
            token = strtok(NULL, " \t");
        }
        argv[argc] = NULL;

        // detect redirection
        bool redirectOut = false;
        bool redirectIn = false;
        string outfileName = "";
        string infileName = "";

        for (int i = 0; i < argc; i++) {

            if (string(argv[i]) == ">") {
                redirectOut = true;
                outfileName = argv[i+1];
                argv[i] = NULL;
                break;
            }

            if (string(argv[i]) == "<") {
                redirectIn = true;
                infileName = argv[i+1];
                argv[i] = NULL;
                break;
            }
        }

        // convert args
        vector<string> cmdArgs;
        for (int i = 0; argv[i] != NULL; i++)
            cmdArgs.push_back(string(argv[i]));

        // OUTPUT REDIRECTION
        if (redirectOut) {

            int fd = open(outfileName.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);

            int saved = dup(STDOUT_FILENO);
            dup2(fd, STDOUT_FILENO);

            runCommand(cmdArgs);

            fflush(stdout);
            dup2(saved, STDOUT_FILENO);

            close(fd);
            close(saved);
        }

        // INPUT REDIRECTION
        else if (redirectIn) {

            int fd = open(infileName.c_str(), O_RDONLY);

            int saved = dup(STDIN_FILENO);
            dup2(fd, STDIN_FILENO);

            string output = runCommand(cmdArgs);
            cout << output;

            dup2(saved, STDIN_FILENO);

            close(fd);
            close(saved);
        }

        // NORMAL COMMAND
        else {

            string output = runCommand(cmdArgs);
            cout << output;
        }
    }

    cout << "Exiting shell.\n";
    return 0;
}