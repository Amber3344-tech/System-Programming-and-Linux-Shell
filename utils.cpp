#include <string>
#include <vector>
#include <fstream>
#include <unistd.h>
#include <sys/wait.h>
using namespace std;

vector<string> gethistory(string filename) {
    vector<string> history;
    ifstream infile(filename);
    string line;
    while (getline(infile, line)) {
        history.push_back(line);
    }
    infile.close();
    return history;
}

string printWorkingDirectory() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        return string(cwd);
    } else {
        perror("getcwd() error");
        return "";
    }
}

// execute a command via fork/execvp and capture its output through a pipe
string runCommand(const vector<string>& args) {
            // string comparison to check for "exit" command

    if (args.empty())
        return string();

    int fd[2];
    if (pipe(fd) == -1) {
        perror("pipe");
        return string();
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        close(fd[0]);
        close(fd[1]);
        return string();
    }

    if (pid == 0) {
        // child
        close(fd[0]);              // close unused read end
        dup2(fd[1], STDOUT_FILENO); // redirect stdout to pipe
        dup2(fd[1], STDERR_FILENO); // optionally capture stderr
        close(fd[1]);

        // prepare argv array
        vector<char*> argv;
        for (const auto &s : args)
            argv.push_back(const_cast<char*>(s.c_str()));
        argv.push_back(NULL);

        execvp(argv[0], argv.data());
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        // parent
        close(fd[1]); // close write end
        string output;
        char buffer[4096];
        ssize_t count;
        while ((count = read(fd[0], buffer, sizeof(buffer))) > 0) {
            output.append(buffer, count);
        }
        close(fd[0]);

        int status;
        waitpid(pid, &status, 0);
        return output;
    }
}