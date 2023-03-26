///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Application description:
//      Small server using TCP socket type, that listen on port 9000.
//      The server shall get all the data from client and store it into a file
//      "/var/tmp/aesdsocketdata" and when done, it will send back all the received data to the
//      client.
//
//      Note: The application handle SIGINT and SIGTERM using a sigaction
//      
//      TODO: need to support multiple client and use non-blocking io.........InProgress
//      TODO: need to handle data from socket.................................InProgress
//      TODO: need to write data into a file..................................TBD
//      TODO: need to send data back to the client............................TBD
//      TODO: need to handle argument -d and launch it as a daemon............TBD
//      TODO: do we need to support multiple client???........................TBD
//
//  Note: In short, ssize_t is the same as size_t, but is a signed type - read ssize_t as
//          “signed size_t”. ssize_t is able to represent the number -1, which is returned by
//          several system calls and library functions as a way to indicate error. For example, the
//          read and write system calls
//
//          size_t == long unsigned int
//
//  Usage:  ./aesdsocket            To run it without a daemon
//          ./aesdsocket -d         To run it as a deamon
//
///////////////////////////////////////////////////////////////////////////////////////////////////

// -- Include -------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>              // To use fcntl()
#include <assert.h>

// -- Global Constants ----------------------------------------------------------------------------
#define PORT 9000
#define CONNECTION_QUEUE 3
#define OUTPUT_FILE "/var/tmp/aesdsocketdata"
#define RECV_BUFFER_SIZE 1024
//#define PROC_BUFFER_SIZE 1024

// -- Flags ---------------------------------------------------------------------------------------
volatile int running = 1;               // TODO: change flag for bool instead of int

bool caught_sigint = false;
bool caught_sigterm = false;
//bool caught_sigkill = false;        // Note that SIGKILL signal can not be caught and handle!!
int  caught_another_sig = -1;


//////////////////////////////////////////
// Signal handler function
//////////////////////////////////////////
void handle_signal(int signal_number)
{
    printf("handle_signal()...got signal_number=%d\n", signal_number);            // To be commentted out!!!

    // Flag which signal we received
    if (signal_number == SIGINT)
        caught_sigint = true;
    else if (signal_number == SIGTERM)
        caught_sigterm = true;
    //else if (signal_number == SIGKILL)                  // Note that SIGKILL signal can not be caught and handle!!
    //    caught_sigkill = true;
    else
        caught_another_sig = signal_number;

    // Set global flag
    running = 0;
}

//////////////////////////////////////////
// Function to print infos received about received Signal
//////////////////////////////////////////
void print_signal_infos()
{
    printf("print_signal_infos()...\n");            // To be commentted out!!!

    if (caught_sigint)
        printf("\nCaught SIGINT!\n");

    if (caught_sigterm)
        printf("\nCaught SIGTERM!\n");

    //if (caught_sigkill)                                   // Note that SIGKILL signal can not be caught and handle!!
    //    printf("\nCaught SIGKILL!\n");

    if (caught_another_sig != -1)
        printf("\nCaught Signal no %d\n", caught_another_sig);
}


void delete_output_file()
{
    int result = remove(OUTPUT_FILE);

    if (result == 0) {
        printf("File deleted successfully.\n");
    } else {
        printf("Error deleting file.\n");
    }
}


//////////////////////////////////////////
// Main Server entry point
//////////////////////////////////////////
//int main()
int main(int argc, char *argv[])
{
    FILE *pFile;		                // Creating file pointer to work with files
    int socket_fd;                      // Socket file descriptor used by the Server                                        //server_fd;
    int new_socket = -1;                // Socket file descriptor used to communicate with a client application         // *** to be rename client_socket ***
    struct sockaddr_in sSockAddress;                                                                                        //address
    int opt = 1;
    int addrlen = sizeof(sSockAddress);
    //char buffer[1024] = {0};            // TO be remove
    char recv_buffer[RECV_BUFFER_SIZE] = {0};
    //char proc_buffer[PROC_BUFFER_SIZE] = {0};
    //char bufferTmp[1024] = {0};
    //char bufferTmp[10] = {'H', 'e', 'l', 'l', 'o', '\0'};
    //const char *hello = "Hello from server";
    bool bRunAsDaemon = false;
    //bool bSigHandlerSuccess = true;
    int iUsleepRc = 0;
    ssize_t num_bytes_received;
    ssize_t num_bytes_sent;

    char client_ip[INET_ADDRSTRLEN];
    int client_port = -1;

    bool bSendDataBackToClient = false;


    printf("main()...entry point...argc=%d\n", argc);            // To be commentted out!!!

    // Open syslog
    openlog(NULL, 0, LOG_USER);

    syslog(LOG_DEBUG, "main()...entry point...argc=%d", argc);

    if (argc > 2)
    {
        fprintf(stderr, "Usage: %s.........to run it as an application\n", argv[0]);
        fprintf(stderr, "Usage: %s -d......to run it as a daemon\n", argv[0]);

        // log system LOG_ERR
        syslog(LOG_ERR, "Error! Invalid number of arguments: %d", argc);
        exit(EXIT_FAILURE);
    }
    else 
    {
        if ((argc == 2) && (strcmp( argv[1], "-d") == 0))
            bRunAsDaemon = true;
    }
    syslog(LOG_DEBUG, "main()...bRunAsDaemon=%d", bRunAsDaemon);

    //printf("main()...set signal handler...\n");            // To be commentted out!!!
    syslog(LOG_DEBUG, "main()...set signal handler...");

    // Set sigaction to hanlde Signals
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    // Register signal handlers for SIGTERM, SIGINT, SIGKILL, and SIGQUIT
    if (sigaction(SIGTERM, &sa, NULL) != 0)
    {
        //printf("Error %d (%s) registering for SIGTERM\n", errno, strerror(errno));
        syslog(LOG_ERR, "Failed registering for SIGTERM, errno %d (%s)", errno, strerror(errno));
        //bSigHandlerSuccess = false;
    }

    if (sigaction(SIGINT, &sa, NULL) != 0)
    {
        //printf("Error %d (%s) registering for SIGINT\n", errno, strerror(errno));
        syslog(LOG_ERR, "Failed registering for SIGINT, errno %d (%s)", errno, strerror(errno));
        //bSigHandlerSuccess = false;
    }

    //if (sigaction(SIGKILL, &sa, NULL) != 0)                                           // We can not handle SIGKILL signal!!
    //{
    //    printf("Error %d (%s) registering for SIGKILL\n", errno, strerror(errno));
    //    //bSigHandlerSuccess = false;
    //}

    if (sigaction(SIGQUIT, &sa, NULL) != 0)
    {
        //printf("Error %d (%s) registering for SIGQUIT\n", errno, strerror(errno));
        syslog(LOG_ERR, "Failed registering for SIGQUIT, errno %d (%s)", errno, strerror(errno));
        //bSigHandlerSuccess = false;
    }


    // Opening file in writing mode
    pFile = fopen(OUTPUT_FILE, "a+");

    if (pFile == NULL) {
        //printf("Error! Unable to open file");
        //syslog(LOG_ERR, "Error! Unable to open (or create) file %s", OUTPUT_FILE);
        syslog(LOG_ERR, "Failed to open (or create file %s, errno %d (%s)", OUTPUT_FILE, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    //printf("main()...set socket...\n");            // To be commentted out!!!
    syslog(LOG_DEBUG, "main()...set socket...");

    // Creating socket file descriptor
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        //perror("socket failed");
        syslog(LOG_ERR, "Failed to create server socket, errno %d (%s)", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Set some socket option
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        //perror("setsockopt failed");
        syslog(LOG_ERR, "Failed to set socket options, errno %d (%s)", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
    

    syslog(LOG_DEBUG, "main()...bind and listen to socket...");

    // Forcefully attaching socket to the port 9000
    sSockAddress.sin_family = AF_INET;
    sSockAddress.sin_addr.s_addr = INADDR_ANY;
    sSockAddress.sin_port = htons(PORT);
    if (bind(socket_fd, (struct sockaddr *) &sSockAddress, sizeof(sSockAddress)) < 0)
    {
        //perror("bind failed");
        syslog(LOG_ERR, "Failed to bind socket on port %d, errno %d (%s)", PORT, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (listen(socket_fd, CONNECTION_QUEUE) < 0)
    {
        //perror("listen failed");
        syslog(LOG_ERR, "Failed to listen on socket, errno %d (%s)", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }


    //printf("main()...set socket non-blocking...\n");            // To be commentted out!!!
    syslog(LOG_DEBUG, "main()...set socket non-blocking...");

    // Set the server socket to non-blocking mode
    int flags = fcntl(socket_fd, F_GETFL, 0);
    assert(flags != -1);
    fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
    

    int loop_number = 1;
    while (running)
    {
        //printf("Main loop number= %d, new_socket= %d\n", loop_number, new_socket);
        syslog(LOG_DEBUG, "====> Main loop number= %d, new_socket= %d", loop_number, new_socket);

        if (new_socket == -1)
        {
            // Look for new client connection
            new_socket = accept(socket_fd, (struct sockaddr *) &sSockAddress, (socklen_t *) &addrlen);
            //printf("Main loop...Debug only...new_socket= %d\n", new_socket);
            syslog(LOG_DEBUG, "Main loop...Debug only...new_socket= %d", new_socket);

            if (new_socket == -1)
                //printf("Main loop...Debug only...on accept: Error %d (%s)\n", errno, strerror(errno));
                syslog(LOG_ERR, "Failed on accept(), errno %d (%s)", errno, strerror(errno));
            else
            {
                // Extract client IP address and port
                client_ip[INET_ADDRSTRLEN] = {0};       // Re-init buffer
                inet_ntop(AF_INET, &(sSockAddress.sin_addr), client_ip, INET_ADDRSTRLEN);
                client_port = ntohs(sSockAddress.sin_port);

                //printf("New connection from %s:%d\n", client_ip, client_port);
                syslog(LOG_DEBUG, "+++++++++++++++++++++++++++ New connection from %s:%d", client_ip, client_port);

                // // Send a message to the client using the client socket
                // char *message = "Hello, client!";
                // send(new_socket, message, strlen(message), 0);
            }
        }

        if (new_socket >= 0)
        //else
        {
            //printf("Main loop...Debug only...a client is connected, new_socket = %d\n", new_socket);

            // Get data from Socket
            //
            //      ssize_t recv(int sockfd, void *buf, size_t len, int flags);
            //
            //int valread = -1;
            //buffer[1024] = {0};         // re-init buffer, clear buffer
            //valread = read(new_socket, buffer, 1024);
            //valread = recv(new_socket, buffer , 1024 , 0);
            //num_bytes_received = recv(new_socket, buffer , 1024 , 0);

            syslog(LOG_DEBUG, "Look for received data on client socket...");

            memset(recv_buffer, 0, sizeof(recv_buffer)); // clear buffer
            //num_bytes_received = recv(new_socket, recv_buffer, sizeof(recv_buffer), 0);
            num_bytes_received = recv(new_socket, recv_buffer, RECV_BUFFER_SIZE, 0);
            
            //printf("Num byte received is: %ld\n", num_bytes_received);
            syslog(LOG_DEBUG, "Num byte received is: %ld", num_bytes_received);

            if (num_bytes_received > 0)
            {
                //printf("Buffer contains     : %s\n", buffer);
                syslog(LOG_DEBUG, "Received buffer contains: %s", recv_buffer);
                syslog(LOG_DEBUG, "Received buffer strlen(): %ld", strlen(recv_buffer));
                size_t num_bytes_to_write = (size_t)num_bytes_received;
                syslog(LOG_DEBUG, "num_bytes_to_write: %ld", num_bytes_to_write);
                

                // set flag to return (or not) the data to the client
                bSendDataBackToClient = false;
                if (num_bytes_to_write < RECV_BUFFER_SIZE)
                {
                    // packet is complete, send all data back to client
                    bSendDataBackToClient = true;
                }
                syslog(LOG_DEBUG, "bSendDataBackToClient: %d", bSendDataBackToClient);

                // TODO: Append data to file
                // TODO create File, open file, etc... and append to file
                //  size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
                //
                            // for (int i = 0; i < valread; i++)
                            // {
                            //     bufferTmp[i] = buffer[i];
                            // }
                            // printf("After copy, bufferTmp contains     : %s\n", bufferTmp);

                // Write into the file
                //int num_char_written = fprintf(pFile, "%s", recv_buffer);
                //if (num_char_written > 0)
                //{
                //    syslog(LOG_DEBUG, "Writing %s to %s", recv_buffer, OUTPUT_FILE);
                //}
                // Write buffer contents to file
                size_t num_bytes_written  = fwrite(recv_buffer, 1, num_bytes_to_write, pFile);
                syslog(LOG_DEBUG, "num_bytes_written: %ld", num_bytes_written);

                if (num_bytes_to_write != num_bytes_written)
                {
                    syslog(LOG_ERR, "Failed to write all buffer to file, errno %d (%s)", errno, strerror(errno));
                }
                else
                {
                    syslog(LOG_DEBUG, "All good all buffer written to file...");
                }



                // TODO: Send the full content of the file back to the client   (COMMENTED OUT TEMPORARY)
                //      ssize_t read(int fd, void *buf, size_t count);                      // read up to 'count' byte and return the number of byte read
                //      ssize_t send(int sockfd, const void *buf, size_t len, int flags);   //
                
                if (bSendDataBackToClient)
                {
                    //printf("Prepare to send the full content of the file back to the client...\n");
                    //
                    //      size_t fread(void *ptr, size_t size, size_t nmembFILE *" stream );
                    //
                    syslog(LOG_DEBUG, "Send the full content of the file back to the client...");

                    // // //send(new_socket, hello, strlen(hello), 0);
                    // // //printf("Hello message sent\n");
                    // // int num_bytes_sent = 0;
                    // // //num_bytes_sent = send(new_socket, buffer, valread, 0);
                    // printf("Buffer contains     : %s\n", buffer);
                    // num_bytes_sent = send(new_socket, buffer, strlen(buffer), 0);
                    // printf("Num byte sent is: %ld\n", num_bytes_sent);

                    // if (num_bytes_sent == -1)
                    // {
                    //     printf("Main loop...Debug only...on send: Error %d (%s)\n", errno, strerror(errno));
                    // }
                        

                    // // // Send a message to the client using the client socket                
                    // //char *message = "Hello, client!!!";
                    // //send(new_socket, message, strlen(message), 0);

                    // Determine file size
                    fseek(pFile, 0, SEEK_END);
                    long file_size = ftell(pFile);
                    syslog(LOG_DEBUG, "file_size: %ld", file_size);
                    fseek(pFile, 0, SEEK_SET);

                    // Allocate buffer for file contents
                    char *my_tmp_buffer = (char *)malloc(file_size + 1);

                    if (my_tmp_buffer == NULL)
                    {
                        syslog(LOG_ERR, "Failed on malloc(), errno %d (%s)", errno, strerror(errno));
                        // Handle error allocating buffer
                        //fclose(pFile);
                    }
                    else
                    {
                        // Read file contents into buffer
                        syslog(LOG_DEBUG, "Copy file content into a buffer...");
                        fread(my_tmp_buffer, 1, file_size, pFile);
                        my_tmp_buffer[file_size] = '\0'; // Null-terminate buffer
                        //printf("%s", my_tmp_buffer);
                        syslog(LOG_DEBUG, "Tmp buffer contains: %s", my_tmp_buffer);

                        syslog(LOG_DEBUG, "Send buffer content back to the client...");
                        num_bytes_sent = send(new_socket, my_tmp_buffer, strlen(my_tmp_buffer), 0);
                        // printf("Num byte sent is: %ld\n", num_bytes_sent);
                        syslog(LOG_DEBUG, "Num byte sent is: %ld", num_bytes_sent);


                        // Free buffer and close file
                        free(my_tmp_buffer);
                        //fclose(file);
                    }

                }
                else
                {
                    syslog(LOG_DEBUG, "Packet not completed yet, do not send it back to client now...");
                }
                
            }
            else
            {
                // if valread == 0      // End of connection
                // if valread == -1     // Handle error
                //printf("Main loop...Debug only...new_socket= %d\n", new_socket);

                if (num_bytes_received == 0)
                {
                    // End of connection
                    syslog(LOG_DEBUG, "------------------------------- Closed connection from %s:%d", client_ip, client_port);

                    if (close(new_socket) != 0)
                        syslog(LOG_ERR, "Failed closing client socket, errno %d (%s)", errno, strerror(errno));
                    else
                        new_socket = -1;        // Re-init client_socket file descriptor
                        
                    //break;                            // TODO cparadis : do we really need it??? Yes we need to handle 'close connection
                }
                else if (num_bytes_received == -1)
                {
                    syslog(LOG_ERR, "Need to handle error here..., errno %d (%s)", errno, strerror(errno));
                }
            }
        }
        
        loop_number += 1;               // Update loop_number
        //sleep(1);                     // Sleep 1 sec... to be remove!!!
        //iUsleepRc = usleep(500000);     // Sleep 500ms... to be remove!!!
        //iUsleepRc = usleep(100000);     // Sleep 100ms... to be remove!!!
        iUsleepRc = usleep(10000);     // Sleep 10ms... to be remove!!!
        if (iUsleepRc != 0)
        {
            //printf("Main loop...Debug only...on usleep %d (%s)\n", errno, strerror(errno));
            syslog(LOG_ERR, "Failed on usleep(), errno %d (%s)", errno, strerror(errno));
            //break;      // Exit while loop
        }
    }


    // TODO: Send data back to the client   (COMMENTED OUT TEMPORARY)
    //      ssize_t send(int sockfd, const void *buf, size_t len, int flags);
    // TODO : -check return value on send...
    //send(new_socket, hello, strlen(hello), 0);
    //printf("Hello message sent\n");
    //printf("Prepare to send data back to the client...\n");

    // printf("bufferTmp contains     : %s\n", bufferTmp);
    // printf("strlen(bufferTmp)      : %ld\n", strlen(bufferTmp));

    // int num_bytes_sent = 0;
    // //num_bytes_sent = send(new_socket, buffer, valread, 0);
    // num_bytes_sent = send(new_socket, bufferTmp, strlen(bufferTmp), 0);
    // //num_bytes_sent = send(socket_fd, bufferTmp, strlen(bufferTmp), 0);
    // printf("Num byte sent is: %d\n", num_bytes_sent);

    // if (num_bytes_sent == -1)
    //     printf("Main loop...Debug only...on send: Error %d (%s)\n", errno, strerror(errno));


    // TODO: Cleanup before leaving
    //  close connection...
    //  closing any open sockets
    //  close file and deleting the file /var/tmp/aesdsocketdata.
    //printf("Cleanup before exit...\n");
    syslog(LOG_DEBUG, "Cleanup before exit...");
    if (new_socket != -1)
    {
        if (close(new_socket) != 0)
            syslog(LOG_ERR, "Failed closing client socket, errno %d (%s)", errno, strerror(errno));
    }
            
    if (close(socket_fd) != 0)
        syslog(LOG_ERR, "Failed closing server socket, errno %d (%s)", errno, strerror(errno));

    
    fclose(pFile);
    // TODO : delete file...
    //delete_output_file();
    print_signal_infos();

    return EXIT_SUCCESS;
}
