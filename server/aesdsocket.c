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
//      TODO: need to support client and use non-blocking socket..............DONE  (we do not need to support multiple client at the same time)
//      TODO: need to handle data from socket.................................DONE
//      TODO: need to write data into a file..................................DONE
//      TODO: need to send the full file content back to the client...........DONE (only when the received packet is completed)
//      TODO: need to handle argument -d and launch it as a daemon............TBD
//
//  Note: In short, ssize_t is the same as size_t, but is a signed type - read ssize_t as
//          “signed size_t”. ssize_t is able to represent the number -1, which is returned by
//          several system calls and library functions as a way to indicate error. For example, the
//          read and write system calls
//
//              size_t  == long unsigned int
//              ssize_t == long signed int 
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
#include <fcntl.h>              // To use fcntl() for socket option
#include <assert.h>

// -- Global Constants ----------------------------------------------------------------------------
#define PORT 9000
#define CONNECTION_QUEUE 3
#define OUTPUT_FILE "/var/tmp/aesdsocketdata"
#define RECV_BUFFER_SIZE 1024

// Optional: use these functions to add debug or error prints to your application
//#define DEBUG_LOG(msg,...)
#define DEBUG_LOG(msg,...) printf("LOG_DEBUG: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("LOG_ERROR: " msg "\n" , ##__VA_ARGS__)


// -- Flags ---------------------------------------------------------------------------------------
volatile int running = 1;                       // TODO: change flag for bool instead of int

bool caught_sigint = false;
bool caught_sigterm = false;
//bool caught_sigkill = false;                  // Note that SIGKILL signal can not be caught and handle!!
int  caught_another_sig = -1;



//////////////////////////////////////////
// Register signal handlers for SIGTERM, SIGINT, and SIGQUIT
//
// Note that we can not register a signal handler for SIGKILL
//////////////////////////////////////////
void register_signal_handler(struct sigaction *sa)
{
    DEBUG_LOG("register_signal_handler()...");

    // Ensure valid param
    assert(sa != NULL); 

    // Register signal handlers for SIGTERM, SIGINT, SIGKILL, and SIGQUIT
    if (sigaction(SIGTERM, sa, NULL) != 0)
    {
        syslog(LOG_ERR, "Failed registering for SIGTERM, errno %d (%s)", errno, strerror(errno));
    }

    if (sigaction(SIGINT, sa, NULL) != 0)
    {
        syslog(LOG_ERR, "Failed registering for SIGINT, errno %d (%s)", errno, strerror(errno));
    }

    if (sigaction(SIGQUIT, sa, NULL) != 0)
    {
        syslog(LOG_ERR, "Failed registering for SIGQUIT, errno %d (%s)", errno, strerror(errno));
    }

}

//////////////////////////////////////////
// Signal handler function
//////////////////////////////////////////
void handle_signal(int signal_number)
{
    DEBUG_LOG("handle_signal()...got signal_number=%d", signal_number);

    // Flag which signal we received
    if (signal_number == SIGINT)
        caught_sigint = true;
    else if (signal_number == SIGTERM)
        caught_sigterm = true;
    //else if (signal_number == SIGKILL)        // Note that SIGKILL signal can not be caught and handle!!
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
    DEBUG_LOG("print_signal_infos()...");

    if (caught_sigint)
        DEBUG_LOG("print_signal_infos()...Caught SIGINT!");

    if (caught_sigterm)
        DEBUG_LOG("print_signal_infos()...Caught SIGTERM!");

    if (caught_another_sig != -1)
        DEBUG_LOG("print_signal_infos()...Caught Signal no %d", caught_another_sig);

    syslog(LOG_ERR, "Caught signal, exiting");
}


//////////////////////////////////////////
// Function to delete the output file
//////////////////////////////////////////
void delete_output_file()
{
    DEBUG_LOG("delete_output_file()...");

    int result = remove(OUTPUT_FILE);

    if (result == 0)
    {
        DEBUG_LOG("delete_output_file()...File deleted successfully.");
    }
    else
    {
        ERROR_LOG("delete_output_file()...Failed to delete file %s, errno %d (%s)", OUTPUT_FILE, errno, strerror(errno));
        syslog(LOG_ERR, "Failed to delete file %s, errno %d (%s)", OUTPUT_FILE, errno, strerror(errno));
    }
}


//////////////////////////////////////////
// Main Server entry point
//////////////////////////////////////////
int main(int argc, char *argv[])
{
    FILE *pFile;		                    // Creating file pointer to work with files
    int socket_fd;                          // Socket file descriptor used by the Server                                        //server_fd;
    int new_socket = -1;                    // Socket file descriptor used to communicate with a client application         // *** to be rename client_socket ***
    struct sockaddr_in sSockAddress;        // Socket addr structure
    int opt = 1;
    int addrlen = sizeof(sSockAddress);
    //char buffer[1024] = {0};              // To be remove
    char recv_buffer[RECV_BUFFER_SIZE] = {0};
    bool bRunAsDaemon = false;
    ssize_t num_bytes_received;
    
    char client_ip[INET_ADDRSTRLEN];        // Used to extract client IP addr
    int client_port = -1;                   // Used to extract the client port

    bool bSendDataBackToClient = false;
    ssize_t num_bytes_sent;

    int iUsleepRc = 0;                      // TODO: to be remove!!
    

    DEBUG_LOG("main()...entry point...argc=%d", argc);

    // Open syslog
    openlog(NULL, 0, LOG_USER);
    
    // Handle parameters
    if (argc > 2)
    {
        printf("Usage: %s          to run it as an application\n", argv[0]);
        printf("Usage: %s -d       to run it as a daemon\n", argv[0]);
        
        // log system LOG_ERR
        syslog(LOG_ERR, "Error! Invalid number of arguments: %d", argc);
        exit(EXIT_FAILURE);
    }
    else 
    {
        // Set flag for daemon process
        if ((argc == 2) && (strcmp( argv[1], "-d") == 0))
            bRunAsDaemon = true;
    }
    DEBUG_LOG("main()...bRunAsDaemon=%d", bRunAsDaemon);

    
    // Set sigaction to hanlde Signals
    DEBUG_LOG("main()...set signal handler...");
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    register_signal_handler(&sa);


    // Opening file in 'append' mode
    DEBUG_LOG("main()...open output file...");
    pFile = fopen(OUTPUT_FILE, "a+");

    if (pFile == NULL)
    {
        syslog(LOG_ERR, "Failed to open (or create) file %s, errno %d (%s)", OUTPUT_FILE, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }


    // Creating socket file descriptor
    DEBUG_LOG("main()...create and set server socket...");
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        syslog(LOG_ERR, "Failed to create server socket, errno %d (%s)", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Set some socket option
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        syslog(LOG_ERR, "Failed to set socket options, errno %d (%s)", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
    

    // Forcefully attaching socket to the port 9000
    DEBUG_LOG("main()...bind and listen to socket...");
    sSockAddress.sin_family = AF_INET;
    sSockAddress.sin_addr.s_addr = INADDR_ANY;
    sSockAddress.sin_port = htons(PORT);

    if (bind(socket_fd, (struct sockaddr *) &sSockAddress, sizeof(sSockAddress)) < 0)
    {
        syslog(LOG_ERR, "Failed to bind socket on port %d, errno %d (%s)", PORT, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (listen(socket_fd, CONNECTION_QUEUE) < 0)
    {
        syslog(LOG_ERR, "Failed to listen on socket, errno %d (%s)", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    
    // Set the server socket to non-blocking mode
    DEBUG_LOG("main()...set socket non-blocking...");
    int flags = fcntl(socket_fd, F_GETFL, 0);
    assert(flags != -1);
    fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
    

    int loop_number = 1;
    while (running)
    {
        //printf("Main loop number= %d, new_socket= %d\n", loop_number, new_socket);
        syslog(LOG_DEBUG, "====> Main loop number= %d, new_socket= %d", loop_number, new_socket);
        //DEBUG_LOG("main()...====> Main loop number= %d, new_socket= %d", loop_number, new_socket);

        if (new_socket == -1)
        {
            // Look for new client connection
            new_socket = accept(socket_fd, (struct sockaddr *) &sSockAddress, (socklen_t *) &addrlen);
            //printf("Main loop...Debug only...new_socket= %d\n", new_socket);
            syslog(LOG_DEBUG, "Main loop...Debug only...new_socket= %d", new_socket);
            //DEBUG_LOG("main()...====> Main loop...new_socket= %d", new_socket);

            if (new_socket == -1)
                syslog(LOG_ERR, "Failed on accept(), errno %d (%s)", errno, strerror(errno));
            else
            {
                // Extract client IP address and port
                //client_ip[INET_ADDRSTRLEN] = {0};       // Re-init buffer
                memset(client_ip, 0, sizeof(client_ip)); // clear buffer
                inet_ntop(AF_INET, &(sSockAddress.sin_addr), client_ip, INET_ADDRSTRLEN);
                client_port = ntohs(sSockAddress.sin_port);

                syslog(LOG_DEBUG, "+++++++++++++++++++++++++++ New connection from %s:%d", client_ip, client_port);
            }
        }

        if (new_socket >= 0)
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
    }   // End while (running)


    // TODO: Cleanup before leaving
    //  close connection...
    //  closing any open sockets
    //  close file and deleting the file /var/tmp/aesdsocketdata.
    syslog(LOG_DEBUG, "Cleanup before exit...");
    if (new_socket != -1)
    {
        if (close(new_socket) != 0)
            syslog(LOG_ERR, "Failed closing client socket, errno %d (%s)", errno, strerror(errno));
    }
            
    if (close(socket_fd) != 0)
        syslog(LOG_ERR, "Failed closing server socket, errno %d (%s)", errno, strerror(errno));
    
    fclose(pFile);
    delete_output_file();

    print_signal_infos();

    return EXIT_SUCCESS;
}
