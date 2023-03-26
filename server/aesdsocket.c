///////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Small server app that handle 1 client connection, get data, store it, and send it back.
//
//
//  Usage:  ./aesdsocket            To run it without a daemon
//          ./aesdsocket -d         To run it as a deamon
//
//
// Application description:
//      Small server using TCP socket type, that listen on port 9000.
//          - The server shall get all the data from client and store it into a file
//              "/var/tmp/aesdsocketdata".
//          - When it received a packet complete, then it shall send back to the client the full
//              file content (but only when the packet is completed)
//          - Note that the packet are ended when a newline character is found
//
//      Note: The application handle SIGINT and SIGTERM using a sigaction.....DONE
//      Note: Need to support client and use non-blocking socket..............DONE  (we do not need to support multiple clients at the same time)
//      Note: Need to handle data from socket.................................DONE
//      Note: Need to write data into a file..................................DONE
//      Note: Need to send the full file content back to the client...........DONE  (only when the received packet is completed)
//      TODO: need to handle argument -d and launch it as a daemon............TBD
//
//
//  Note: In short, ssize_t is the same as size_t, but is a signed type - read ssize_t as
//          “signed size_t”. ssize_t is able to represent the number -1, which is returned by
//          several system calls and library functions as a way to indicate error. For example, the
//          read and write system calls
//
//              size_t  == long unsigned int
//              ssize_t == long signed int
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
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("LOG_DEBUG: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("LOG_ERROR: " msg "\n" , ##__VA_ARGS__)


// -- Flags ---------------------------------------------------------------------------------------
volatile int running = 1;               // TODO: change flag for bool instead of int

bool caught_sigint = false;
bool caught_sigterm = false;
//bool caught_sigkill = false;          // Note that SIGKILL signal can not be caught and handle!!
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
    syslog(LOG_ERR, "Caught signal, exiting");
}

//////////////////////////////////////////
// Function to print infos received about received Signal
//////////////////////////////////////////
void print_signal_infos()
{
    DEBUG_LOG("print_signal_infos()...");

    if (caught_sigint)
    {
        DEBUG_LOG("print_signal_infos()...Caught SIGINT!");
        syslog(LOG_ERR, "Caught SIGINT!");
    }
        
    if (caught_sigterm)
    {
        DEBUG_LOG("print_signal_infos()...Caught SIGTERM!");
        syslog(LOG_ERR, "Caught SIGTERM!");
    }
        
    if (caught_another_sig != -1)
    {
        DEBUG_LOG("print_signal_infos()...Caught another Signal no %d", caught_another_sig);
        syslog(LOG_ERR, "Caught Caught another Signal no %d", caught_another_sig);
    }
}



//////////////////////////////////////////
// Main Server entry point
//////////////////////////////////////////
int main(int argc, char *argv[])
{
    FILE *pFile;		                        // File descriptor to work with files
    int server_socket;                          // Socket file descriptor used by the Server
    int client_socket = -1;                     // Socket file descriptor used to communicate with a client application
    struct sockaddr_in sSockAddress;            // Socket addr structure
    int opt = 1;
    int addrlen = sizeof(sSockAddress);

    char client_ip[INET_ADDRSTRLEN];            // Used to extract client IP addr
    int client_port = -1;                       // Used to extract the client port

    char recv_buffer[RECV_BUFFER_SIZE] = {0};   // Received buffer to get data from socket
    ssize_t num_bytes_received;
    
    bool bSendDataBackToClient = false;         // Flag indicating to send data back to client 
    ssize_t num_bytes_sent;                     

    bool bRunAsDaemon = false;                  // Flag to run app as a daemon


    DEBUG_LOG("main()...entry point...argc=%d", argc);

    // Open syslog
    openlog(NULL, 0, LOG_USER);
    
    // Handle parameters
    if (argc > 2)
    {
        printf("Error! Invalid number of arguments: %d\n", argc);
        printf("\n");
        printf("Usage: %s          to run it as an application\n", argv[0]);
        printf("Usage: %s -d       to run it as a daemon\n", argv[0]);
        
        exit(EXIT_FAILURE);
    }
    else 
    {
        // Set flag for daemon process
        if ((argc == 2) && (strcmp( argv[1], "-d") == 0))
            bRunAsDaemon = true;
    }
    printf("main()...bRunAsDaemon=%d\n", bRunAsDaemon);             // TEMPORARY for compilation issue

    
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
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        syslog(LOG_ERR, "Failed to create server socket, errno %d (%s)", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Set some socket option
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        syslog(LOG_ERR, "Failed to set socket options, errno %d (%s)", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
    

    // Forcefully attaching socket to the port 9000
    DEBUG_LOG("main()...bind and listen to socket...");
    sSockAddress.sin_family = AF_INET;
    sSockAddress.sin_addr.s_addr = INADDR_ANY;
    sSockAddress.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *) &sSockAddress, sizeof(sSockAddress)) < 0)
    {
        syslog(LOG_ERR, "Failed to bind socket on port %d, errno %d (%s)", PORT, errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, CONNECTION_QUEUE) < 0)
    {
        syslog(LOG_ERR, "Failed to listen on socket, errno %d (%s)", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    
    // Set the server socket to non-blocking mode
    DEBUG_LOG("main()...set socket non-blocking...");
    int flags = fcntl(server_socket, F_GETFL, 0);
    assert(flags != -1);
    fcntl(server_socket, F_SETFL, flags | O_NONBLOCK);
    

    int loop_number = 1;
    while (running)
    {
        // Do not log...there is too much!!
        //syslog(LOG_DEBUG, "====> Main loop number= %d, client_socket= %d", loop_number, client_socket);
        
        if (client_socket == -1)
        {
            // Look for new client connection
            client_socket = accept(server_socket, (struct sockaddr *) &sSockAddress, (socklen_t *) &addrlen);
            // Do not log...there is too much!!
            //syslog(LOG_DEBUG, "Main loop...Debug only...client_socket= %d", client_socket);
            
            if (client_socket == -1)
            {
                // Do not log...there is too much!!
                //syslog(LOG_ERR, "Failed on accept(), errno %d (%s)", errno, strerror(errno));
            }
            else
            {
                // Extract client IP address and port
                memset(client_ip, 0, sizeof(client_ip)); // clear buffer
                inet_ntop(AF_INET, &(sSockAddress.sin_addr), client_ip, INET_ADDRSTRLEN);
                client_port = ntohs(sSockAddress.sin_port);

                syslog(LOG_DEBUG, "New connection from %s:%d", client_ip, client_port);
            }
        }

        if (client_socket >= 0)
        {
            DEBUG_LOG("main()...====> Main loop...a client is connected, client_socket = %d", client_socket);

            // Get data from Socket
            //
            //      ssize_t recv(int sockfd, void *buf, size_t len, int flags);
            //            
            DEBUG_LOG("main()...====> Main loop...Look for received data on client socket...");

            memset(recv_buffer, 0, sizeof(recv_buffer));    // clear buffer
            num_bytes_received = recv(client_socket, recv_buffer, RECV_BUFFER_SIZE, 0);
            DEBUG_LOG("main()...====> Main loop...Num byte received is: %ld", num_bytes_received);

            if (num_bytes_received > 0)
            {                
                if (num_bytes_received < 256)   // Arbitrary value, just do not output large buffer
                {
                    DEBUG_LOG("main()...====> Main loop...Received buffer contains: %s", recv_buffer);
                }                
                DEBUG_LOG("main()...====> Main loop...Received buffer strlen(): %ld", strlen(recv_buffer));
                size_t num_bytes_to_write = (size_t)num_bytes_received;
                DEBUG_LOG("main()...====> Main loop...num_bytes_to_write: %ld", num_bytes_to_write);
                

                // Set flag to return (or not) the data to the client
                bSendDataBackToClient = false;
                if (num_bytes_to_write < RECV_BUFFER_SIZE)
                {
                    // packet is complete, send all data back to client
                    bSendDataBackToClient = true;
                }
                DEBUG_LOG("main()...====> Main loop...bSendDataBackToClient: %d", bSendDataBackToClient);


                // Write buffer contents to file (in fact, append data at end of file)
                //
                //      size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
                //
                DEBUG_LOG("main()...====> Main loop...Write buffer contents to file...");
                size_t num_bytes_written  = fwrite(recv_buffer, 1, num_bytes_to_write, pFile);
                DEBUG_LOG("main()...====> Main loop...num_bytes_written: %ld", num_bytes_written);

                if (num_bytes_to_write != num_bytes_written)
                {
                    syslog(LOG_ERR, "Failed to write all buffer to file, errno %d (%s)", errno, strerror(errno));
                }
                else
                {
                    DEBUG_LOG("main()...====> Main loop...All good all buffer written to file...");
                }

                
                // Send the full content of the file back to the client (only if packet is completed)
                //
                //      ssize_t read(int fd, void *buf, size_t count);
                //              // read up to 'count' byte and return the number of byte read
                //      ssize_t send(int sockfd, const void *buf, size_t len, int flags);
                //              // send data to socket, the message is found in buf and has length len.
                //
                if (bSendDataBackToClient)
                {
                    //
                    //      int fseek(FILE *stream, long offset, int whence);
                    //      long ftell(FILE *stream);
                    //      size_t fread(void *restrict ptr, size_t size, size_t nmemb, FILE *restrict stream);
                    //
                    DEBUG_LOG("main()...====> Main loop...Send the full content of the file back to the client...");

                    // Determine file size
                    fseek(pFile, 0, SEEK_END);
                    long file_size = ftell(pFile);
                    DEBUG_LOG("main()...====> Main loop...file_size: %ld", file_size);
                    fseek(pFile, 0, SEEK_SET);

                    // Allocate buffer for file contents
                    char *my_tmp_buffer = (char *)malloc(file_size + 1);

                    if (my_tmp_buffer == NULL)
                    {
                        syslog(LOG_ERR, "Failed on malloc(), errno %d (%s)", errno, strerror(errno));
                    }
                    else
                    {
                        // Read file contents into buffer
                        DEBUG_LOG("main()...====> Main loop...Read file content into a buffer...");
                        size_t num_bytes_read = fread(my_tmp_buffer, 1, file_size, pFile);
                        DEBUG_LOG("main()...====> Main loop...num_bytes_read: %ld", num_bytes_read);
                        my_tmp_buffer[file_size] = '\0'; // Null-terminate buffer
                        
                        if (num_bytes_read < 256)   // Arbitrary value, just do not output large file
                        {
                            DEBUG_LOG("main()...====> Main loop...Tmp buffer contains: %s", my_tmp_buffer);
                        }
                        

                        DEBUG_LOG("main()...====> Main loop...Send buffer content back to the client...");
                        num_bytes_sent = send(client_socket, my_tmp_buffer, strlen(my_tmp_buffer), 0);
                        DEBUG_LOG("main()...====> Main loop...num_bytes_sent: %ld", num_bytes_sent);
                        printf("main()...====> Main loop...num_bytes_sent: %ld", num_bytes_sent);       // TEMPORARY for compilation issue


                        // Free buffer and close file
                        free(my_tmp_buffer);
                    }
                }
                else
                {
                    DEBUG_LOG("main()...====> Main loop...Packet not completed yet, do not send it back to client now...");
                }                
            }
            else
            {
                //
                // Handle close connection or error got on recv()
                //
                if (num_bytes_received == 0)        // End of connection
                {
                    syslog(LOG_DEBUG, "Closed connection from %s:%d", client_ip, client_port);

                    if (close(client_socket) != 0)
                        syslog(LOG_ERR, "Failed closing client socket, errno %d (%s)", errno, strerror(errno));

                    client_socket = -1;                // Re-init client_socket file descriptor
                }
                else if (num_bytes_received == -1)   // Handle error
                {
                    syslog(LOG_ERR, "Failed on recv(), need to handle error == %ld, errno %d (%s)", num_bytes_received, errno, strerror(errno));
                }
                else
                {
                    syslog(LOG_ERR, "Failed on recv(), need to handle error == %ld, errno %d (%s)", num_bytes_received, errno, strerror(errno));
                }
            }
        }
        
        loop_number += 1;                   // Update loop_number
        
        // if (usleep(10000) != 0)             // Sleep 10ms... to be remove!!!    // Used for debug only, commented out
        // {
        //     syslog(LOG_ERR, "Failed on usleep(), errno %d (%s)", errno, strerror(errno));
        // }

    }   // End while (running)


    // Cleanup before exiting!!
    //      closing client socket connection (client connection)
    //      closing server socket
    //      closing output file
    //      deleting output file
    DEBUG_LOG("main()...Cleanup before exit...");
    if (client_socket != -1)
    {
        if (close(client_socket) != 0)
            syslog(LOG_ERR, "Failed closing client socket, errno %d (%s)", errno, strerror(errno));
    }
            
    if (close(server_socket) != 0)
        syslog(LOG_ERR, "Failed closing server socket, errno %d (%s)", errno, strerror(errno));
    
    if (fclose(pFile) != 0)
        syslog(LOG_ERR, "Failed closing file, errno %d (%s)", errno, strerror(errno));

    if (remove(OUTPUT_FILE) != 0)
        syslog(LOG_ERR, "Failed to delete file %s, errno %d (%s)", OUTPUT_FILE, errno, strerror(errno));

    print_signal_infos();

    return EXIT_SUCCESS;
}
