#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/types.h>
#include <errno.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[] ) {
    int minicom_pipe_1[2], minicom_pipe_2[2];

    // Create pipes
    if (pipe(minicom_pipe_1) < 0 || pipe(minicom_pipe_2) < 0) {
        fprintf(stderr, "Failed to create pipes\n");
        return 1;
    }
    
    pid_t pid = fork();

    if (pid < 0) {
        fprintf(stderr, "Failed to fork\n");
        return 1;
    } else if (pid == 0) {
        // Child process will be minicom and communicate with device
        close(minicom_pipe_1[1]); // close pipe read end
        close(minicom_pipe_2[0]); // close pipe write end
        dup2(minicom_pipe_1[0], STDIN_FILENO); // replace write end with std in
        dup2(minicom_pipe_2[1], STDOUT_FILENO); // replace read end with std out 
        close(minicom_pipe_1[0]); // close pipe write end
        close(minicom_pipe_2[1]); // close pipe read end
        execlp("minicom", "minicom", "-D", argv[1], NULL);
        fprintf(stderr, "Failed to execute minicom\n");
        exit(1);
    } else {
        // Parent process
        close(minicom_pipe_1[0]);
        close(minicom_pipe_2[1]);
        int to_minicom = minicom_pipe_1[1];
        int from_minicom = minicom_pipe_2[0];

        // Set minicom_pipe_2[0] to non-blocking mode
        int flags = fcntl(from_minicom, F_GETFL, 0);
        fcntl(from_minicom, F_SETFL, flags | O_NONBLOCK);

        // Read from minicom and print to stdout
        char output_buf[BUFFER_SIZE];
        char *cmd[] = {"m","s", "dir\r","sz *\r"};
        int i = 0;
        int kill_program = 0;
        while (1) {
            int n = read(from_minicom, output_buf, sizeof(output_buf));
            if (n < 0 && errno == EAGAIN) {
                // No data available yet, sleep for a bit or send command
                printf(".");
                fflush(stdout);
                if(kill_program){
                    printf("try again\n");
                    exit(1);
                }
                if(i<4){
                    write(to_minicom, cmd[i], strlen(cmd[i]));
                    printf("\n%d> %s\n",i,cmd[i]);
                    fflush(stdout);
                    i++;
                    kill_program = 1;
                }
                usleep(1000000);
            } else if (n < 0) {
                // Some other error occurred
                fprintf(stderr, "Failed to read from pipe\n");
                break;
            } else if (n == 0) {
                // End of file, minicom process has terminated
                break;
            } else {
                printf(">%.*s", n, output_buf);
                if (i == 1 && strstr(output_buf,"Main Menu") != NULL){
                    kill_program = 0;
                } else if (i == 2 && strstr(output_buf,"SZ  file") != NULL){
                    kill_program = 0;
                } else if (i == 3 && strstr(output_buf,"End of Directory") != NULL){
                    kill_program = 0;
                } else if (i == 4 && strstr(output_buf,"Starting zmodem transfer") != NULL){
                    kill_program = 0;
                }
                if(strstr(output_buf,"Transfer complete") != NULL){
                    write(to_minicom, "\r",1);
                    usleep(1000000);
                    write(to_minicom, "x",1);
                    usleep(1000000);
                    write(to_minicom, "\r",1);
                    usleep(1000000);
                    char ctrl_a = 1;
                    write(to_minicom, &ctrl_a, sizeof(ctrl_a));
                    write(to_minicom, "x", 1);
                    write(to_minicom, "\r",1);
                    usleep(1000000);
                }
            }

        }

        close(to_minicom);
        close(from_minicom);


        wait(NULL);

        // Clear the terminal screen
        printf("\033[2J");

        fflush(stdout);
        // Move the cursor to the top-left corner
        printf("\033[H");

        fflush(stdout);
        printf("Child process has terminated\n");
        fflush(stdout);
    }

    return 0;
}
