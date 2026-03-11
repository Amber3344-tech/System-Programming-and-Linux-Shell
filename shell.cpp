#include <stdio.h>
#include <unistd.h>     // fork(), execvp(), dup2(), getcwd()
#include <string.h>     // strtok(), strncpy()
#include <sys/types.h>
#include <sys/wait.h>   // wait()
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <fstream>      // file streams for history file
#include <fcntl.h>      // open() for file redirection
#include "utils.cpp"

using namespace std;

int main()
{
    string line;

    // Load command history from file at shell startup
    vector<string> hisvec = gethistory("history.txt");

    // Main shell loop
    while (true)
    {
        // Display current working directory as the shell prompt
        string cwd = runCommand({"pwd"});
        cwd.pop_back();
        cout << cwd << "> ";

        // Read command line input from the user
        if (!getline(cin, line))
            break;

        // Remove carriage return (for Windows-style line endings)
        if (!line.empty() && line.back() == '\r')
            line.pop_back();

        // Exit shell if user types "exit"
        if (line == "exit")
            break;

        // Ignore empty commands
        if (line.empty())
            continue;

        // Print command history with numbering
        if (line == "history") {
            for (size_t i = 0; i < hisvec.size(); i++) {
                cout << i + 1 << " " << hisvec[i] << endl;
            }
            continue;
        }

        // Add the command to history
        hisvec.push_back(line);

        // Save history to file so it persists between sessions
        ofstream outfile("history.txt");
        for (const auto& cmd : hisvec)
            outfile << cmd << endl;

        // TOKENIZE THE USER COMMAND
        const int MAXARGS = 64;
        char *argv[MAXARGS];
        int argc = 0;

        // Copy input line to a buffer because strtok modifies it
        char buf[1024];
        strncpy(buf, line.c_str(), sizeof(buf)-1);
        buf[sizeof(buf)-1] = '\0';

        // Split input into tokens separated by spaces/tabs
        char *token = strtok(buf, " \t");
        while (token != NULL && argc < MAXARGS - 1) {
            argv[argc++] = token;
            token = strtok(NULL, " \t");
        }
        argv[argc] = NULL;


        // DETECT SPECIAL OPERATORS
        bool redirectOut = false;
        bool redirectIn = false;
        bool hasPipe = false;
        bool appendOut = false;

        string outfileName = "";
        string infileName = "";

        int pipeIndex = -1;

        // Scan tokens to detect <, >, or |
        for (int i = 0; i < argc; i++) {

            // Output redirection
            if (string(argv[i]) == ">") {
                redirectOut = true;
                outfileName = argv[i+1];
                argv[i] = NULL;   // terminate command before >
                break;
            }

            // Input redirection
            if (string(argv[i]) == "<") {
                redirectIn = true;
                infileName = argv[i+1];
                argv[i] = NULL;
                break;
            }

            // Pipe detection
            if (string(argv[i]) == "|") {
                hasPipe = true;
                pipeIndex = i;
                argv[i] = NULL;   // split command into two parts
                break;
            }
            if (string(argv[i]) == ">>") {
                redirectOut = true;
                appendOut = true;
                outfileName = argv[i+1];
                argv[i] = NULL;
                break;
            }
        }

        // Convert arguments to vector<string> for runCommand()
        vector<string> cmdArgs;
        for (int i = 0; argv[i] != NULL; i++)
            cmdArgs.push_back(string(argv[i]));


        // OUTPUT REDIRECTION ( > )
        if (redirectOut) {

            // Open or create output file
            int fd;

            if (appendOut)
                fd = open(outfileName.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0666);
            else
                fd = open(outfileName.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);


            // Save current stdout
            int saved = dup(STDOUT_FILENO);

            // Redirect stdout to the file
            dup2(fd, STDOUT_FILENO);

            // Execute command
            string output = runCommand(cmdArgs);
            cout << output;

            // Restore stdout back to terminal
            fflush(stdout);
            dup2(saved, STDOUT_FILENO);

            close(fd);
            close(saved);
        }

        // INPUT REDIRECTION ( < )
        else if (redirectIn) {

            // Open input file
            int fd = open(infileName.c_str(), O_RDONLY);

            // Save current stdin
            int saved = dup(STDIN_FILENO);

            // Redirect stdin to file
            dup2(fd, STDIN_FILENO);

            // Execute command and print result
            string output = runCommand(cmdArgs);
            cout << output;

            // Restore stdin back to keyboard
            dup2(saved, STDIN_FILENO);

            close(fd);
            close(saved);
        }

        // PIPE OPERATION ( command1 | command2 )
        else if (hasPipe) {

            int fd[2];

            // Create pipe: fd[0] = read end, fd[1] = write end
            pipe(fd);

            pid_t p1 = fork();

            if (p1 == 0) {
                // First child executes command before |

                // Redirect stdout to pipe write end
                dup2(fd[1], STDOUT_FILENO);

                close(fd[0]);
                close(fd[1]);

                execvp(argv[0], argv);

                // If exec fails
                perror("execvp");
                exit(1);
            }

            // Wait for first command to finish
            wait(NULL);

            pid_t p2 = fork();

            if (p2 == 0) {
                // Second child executes command after |

                // Redirect stdin to pipe read end
                dup2(fd[0], STDIN_FILENO);

                close(fd[1]);
                close(fd[0]);

                execvp(argv[pipeIndex+1], &argv[pipeIndex+1]);

                perror("execvp");
                exit(1);
            }

            close(fd[0]);
            close(fd[1]);

            wait(NULL);

            continue;
        }


        // NORMAL COMMAND EXECUTION
        else {

            // Execute command normally using fork/exec
            string output = runCommand(cmdArgs);
            cout << output;
        }
    }

    cout << "Exiting shell.\n";
    return 0;
}