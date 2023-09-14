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

void run_mincom(int minicom_pipe_1[2], int minicom_pipe_2[2], char* comport) {
    close(minicom_pipe_1[1]); // close pipe write end
    close(minicom_pipe_2[0]); // close pipe read end
    dup2(minicom_pipe_1[0], STDIN_FILENO); // redirect read end to std in
    dup2(minicom_pipe_2[1], STDOUT_FILENO); // redirect write end to std out 
    close(minicom_pipe_1[0]); // close pipe read end
    close(minicom_pipe_2[1]); // close pipe write end
    execlp("minicom", "minicom", "-D", comport, NULL);
    fprintf(stderr, "Failed to execute minicom\n");
    exit(1);
}

void minicom_automation(int minicom_pipe_1[2], int minicom_pipe_2[2],char* file_names[], int num_files) {
    // Parent process
    close(minicom_pipe_1[0]); // close read and of pipe
    close(minicom_pipe_2[1]); //close write end of pipe
    int to_minicom = minicom_pipe_1[1];
    int from_minicom = minicom_pipe_2[0];

    // Set from_minicom to non-blocking mode so that it doesn't wait forever 
    // to receive something from minicom when reading the pipe
    int flags = fcntl(from_minicom, F_GETFL, 0);
    fcntl(from_minicom, F_SETFL, flags | O_NONBLOCK);

    // Read from minicom and print to stdout
    char output_buf[BUFFER_SIZE];
    char ctrl_a = 1;
    int file_index = 0;
    // command to send to minicom: "sz file_name"
    char* szcmd = malloc(sizeof(char) * (strlen(file_names[file_index]) + 5));
    strcpy(szcmd,"sz ");
    strcat(szcmd,file_names[file_index]);
    strcat(szcmd,"\r");
    szcmd[strlen(szcmd)] = '\0';
    file_index++;   
    char *cmd[] = {"m","s",szcmd,"\r", "x","\r",&ctrl_a,"x","\r"};
    // char *cmd[] = {"m","s","sz *\r","\r", "x","\r",&ctrl_a,"x","\r"};
    char *expResponse[] = {"Main Menu","SZ  file"};
    int cmdIndex = 0, wrongResponse = 0, waitForFiles = 0, busyTime = 0;
    while (1) {
        int n = read(from_minicom, output_buf, sizeof(output_buf));
        if (n < 0) {
            if (errno != EAGAIN){
                // Some other error occurred
                fprintf(stderr, "Failed to read from pipe\n");
                break;
            } else if(!waitForFiles){// send commands if not downloading files
                write(to_minicom, cmd[cmdIndex], strlen(cmd[cmdIndex]));
                printf("%d > %s\n", cmdIndex, cmd[cmdIndex]);
                cmdIndex++;
                busyTime = 0;
            } else {
                // just wait
                printf("\033[A\033[K busy for: %ds\n",busyTime);
                fflush(stdout);
                busyTime++;
            }
            // sleep for a sec
            usleep(1000000);
        } else if (n == 0) {
            // End of file, minicom process has terminated
            printf("Minicom has finished\n");
            break;
        } else {
            printf("%.*s", n, output_buf);
            waitForFiles = (cmdIndex == 3) && !(strstr(output_buf,"Transfer complete") != NULL);
            wrongResponse = (cmdIndex < 2 && strstr(output_buf,expResponse[cmdIndex - 1]) == NULL); 
            fflush(stdout);
            if(wrongResponse){ // the last command sent received an unexpected response
                printf("try again\n");
                exit(1);
            }
            if(waitForFiles && strstr(output_buf,"failed") != NULL){
                printf("file not found\n");
                free(szcmd);
                waitForFiles = 0;
                cmdIndex++;
            }
            if(waitForFiles && strstr(output_buf,"Transfer incomplete") != NULL){
                printf("download incomplete\n");
                free(szcmd);
                exit(1);
            }
            if(!waitForFiles && cmdIndex == 3 && file_index < num_files){
                printf("\n\n\n\nhoooooooooooooo\n\n\n\n\n");fflush(stdout);
                //send "\r" to minicom
                write(to_minicom, cmd[3], strlen(cmd[3]));
                // send next file
                free(szcmd);
                szcmd = malloc(sizeof(char) * (strlen("sz ") + strlen(file_names[file_index]) + 5));
                strcpy(szcmd,"sz ");
                strcat(szcmd,file_names[file_index]);
                strcat(szcmd,"\r");
                szcmd[strlen(szcmd)] = '\0';
                cmd[2] = szcmd;
                cmdIndex = 2;
                file_index++;
            }
                
            
        }
    }
    close(to_minicom);
    close(from_minicom);
    wait(NULL);
}

int main(int argc, char *argv[] ) {
    int num_files = argc - 2;
    char* filenames[num_files];
    if (argc < 2) {
        fprintf(stderr, "usage: zmodemFileTransfer [com port] ([file name])\n");
        exit(1);
    } else if (argc < 3) {
        filenames[0] = "*";
    } else {
        for (int i = 0; i < num_files; i++) {
            filenames[i] = argv[i + 2];
        }
    }
    //  Create 2 pipes :
    //      minicom_pipe_1: minicom <- automation script       
    //      minicom_pipe_2: minicom -> automation script
    int minicom_pipe_1[2], minicom_pipe_2[2];

    // ignore sigpipe signal 
    signal(SIGPIPE, SIG_IGN);

    // Create pipes
    if (pipe(minicom_pipe_1) < 0 || pipe(minicom_pipe_2) < 0) {
        fprintf(stderr, "Failed to create pipes\n");
        return 1;
    }
    
    // create 2 processes one that will run minicom
    // and one that will automate reading and writing to minicom
    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Failed to fork\n");
        return 1;
    } else if (pid == 0) {
        // Child process will be minicom and communicate with device
        run_mincom(minicom_pipe_1, minicom_pipe_2, argv[1]);
    } else {
        // automates reading and writing to minicom
        minicom_automation(minicom_pipe_1, minicom_pipe_2, filenames, num_files);
    }

    return 0;
}
