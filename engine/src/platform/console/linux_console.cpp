#include "platform/predef.h"
#ifdef HKLINUX

#include "console.h"

#include <cstdlib>
#include <cstring>
#include <csignal>
#include <fcntl.h>
#include <sys/stat.h>

namespace hk::platform {

constexpr char const *console_title = "Hikai Log Console";
constexpr char const *console_size = "160x55+330+190"; // COLUMNSxROWS+X+Y
constexpr char const *font = "JetBrainsMonoNF-Regular";
constexpr char const *font_size = "12";

static i32 fd = -1;
static i32 pid = -1;

void alloc_console()
{
    if (system("which xterm > /dev/null 2>&1")) {
        // TODO: no terminal found
        return;
    }

    mkfifo("/tmp/hikaipipe", 0600);

    pid = fork();
    if (pid == 0) {
        // TODO: block all input from user

        execl("/usr/bin/xterm",
            "xterm",                   // Mode
            "-bd", "black",            // Border color
            "-bg", "black",            // Background color
            "-fg", "green",            // Text color
            "-fa", font,               // Font
            "-fs", font_size,          // Font size
            "-geometry", console_size, // Size
            "-T", console_title,       // Title
            "-e", "cat /tmp/hikaipipe",
            NULL);

        exit(EXIT_FAILURE);
    }

    fd = open("/tmp/hikaipipe", O_WRONLY);

    // TODO: figure out how to do this via anonymous pipes

    // i32 pipefd[2];
    // pipe(pipefd);
    //
    // if(fork() == 0) {
    //     // close(0); //CHILD CLOSING stdin
    //     // copies the fd of read end of pipe into its fd i.e 0 (STDIN)
    //     // dup(pipefd[0]);
    //     dup2(pipefd[0], STDIN_FILENO);
    //
    //     close(pipefd[0]);  // Close the unused read end
    //     close(pipefd[1]);  // Close the unused write end
    //
    //     // execl("/usr/bin/kitty", "-c", "NONE", "cat", NULL);
    //
    //     // execl("/usr/bin/xterm",
    //     //     "xterm",                              // Mode
    //     //     "-bd", "black",                       // Border color
    //     //     "-bg", "black",                       // Background color
    //     //     "-fg", "green",                       // Text color
    //     //     "-fa", "JetBrainsMonoNF-Regular",     // Font
    //     //     "-fs", "16",                          // Font size
    //     //     "-geometry", "100x25+330+190",        // COLUMNSxROWS+X+Y
    //     //     "-T", "Hikai Log Console",            // Title
    //     //     "-e", "cat",
    //     //     NULL);
    //
    //     exit(1);
    // }
    //
    // close(pipefd[0]);  // Close the unused read end
    // FILE *log = fdopen(pipefd[1], "w");
    //
    // write(pipefd[1], "Hello from C\n", 13);
    //
    // while (true) { fprintf(log, "%s", "test\n"); }
    //
    // close(pipefd[1]);

    // FILE *fp = popen("xterm -e 'cat /dev/stdin'", "w");
    // FILE *fp = popen("kitty -c NONE cat", "w");
    // FILE *fp = popen("cat", "w");
    // fprintf(fp, "%s", "test\n");
}

void dealloc_console()
{
    close(fd);
    kill(pid, 15);
}

void write_console(const char *text)
{
    if (!text || fd == -1 || pid == -1) { return; }

    write(fd, text, strlen(text));
}

// TODO: finish configuration functions
b8 set_console_size(i16 cols, i16 rows)
{
    return false;
}

b8 set_console_title(const char *title)
{
    return false;
}

} // namespace hk::platform

#endif // HKLINUX
