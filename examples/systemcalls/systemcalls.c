#include "systemcalls.h"
#include <stdlib.h>             // To use system()
#include <unistd.h>             // To use fork()
#include <sys/wait.h>           // To use wait()





/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{
    bool bRetValue = false;     // Assume error state at first.

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
 */

    // Debug only
    printf("Debug only: do_system(): The command is \"%s\"\n", cmd);

    int iReturnCode = system(cmd);
    printf("Debug only: do_system(): iReturnCode is %d\n", iReturnCode);

    // The return value of system() is one of the following:
    //
    // If command is NULL, then a nonzero value if a shell is
    //      available, or 0 if no shell is available.                       ===> command is not NULL
    //
    // If a child process could not be created, or its status could
    //      not be retrieved, the return value is -1 and errno is set to
    //      indicate the error.                                             ===> if != -1 then child process has been created
    //
    // If a shell could not be executed in the child process, then
    //      the return value is as though the child shell terminated by
    //      calling _exit(2) with the status 127.                           ===> if != 127 then shell command has been executed
    //
    // If all system calls succeed, then the return value is the
    //      termination status of the child shell used to execute command.
    //      (The termination status of a shell is the termination status
    //      of the last command it executes.)                               ===> On success, the shell return 0
    //
    //if (iReturnCode != 0 && iReturnCode != -1 && iReturnCode != 127)
    if (iReturnCode == 0)
    {
        bRetValue = true;
    }

    return bRetValue;
}





/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    bool bRetValue = false;     // Assume error state at first.
    printf("Debug only: do_exec(): The count is %d\n", count);

    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
        // Debug only
        printf("Debug only: do_exec(): The command%d is %s\n", i, command[i]);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    //command[count] = command[count];

    va_end(args);



/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
 * 
 * --- notes from CParadis:
 * 
 *      pid_t fork(void);
 *          RETURN VALUE
 *              In parent process, -1 on error, or the Child PID on success
 *              In child process, it is 0
 *              
 *      int execv(const char *path, char *const argv[]);
 *           RETURN VALUE
 *               A successful call to execv does not have a return value because the new process image overlays the calling process image. 
 *               However, a -1 is returned if the call to execv is unsuccessful.
 * 
 *      pid_t wait(int *wstatus);
 *              wait(): on success, returns the process ID of the terminated child; on failure, -1 is returned.
 *              int wstatus;        wait(&wstatus)
 *                  On success, wstatus shall be 0, and non-zero on failure
 * 
 * Reference: https://support.sas.com/documentation/onlinedoc/sasc/doc/lr2/execv.htm
 * 
 */

    pid_t childPid;
    int execvRetCode;
    pid_t waitedChildPid;
    int waitStatus;
    if ((childPid = fork()) == -1 )
    {
        perror("can't fork");
    }
    else if (childPid == 0)
    {
        // Let Child Process to execute any work
        execvRetCode = execv(command[0], command);
        printf("Debug only: do_exec(): The return code is %d, but return is not expected on success\n", execvRetCode);
    }
    else
    {
        // Let Parent process to finish any work
        printf("Debug only: do_exec(): The Child PID is %d\n", childPid);
        
        // Waiting on Child process
        waitedChildPid = wait(&waitStatus);
        
        if (waitedChildPid == -1)
        {
            perror("Got error while waiting on Child process...");
        }
        else
        {
            printf("Debug only: do_exec(): The Waited Child PID is %d\n", waitedChildPid);
            printf("Debug only: do_exec(): The Wait status is %d\n", waitStatus);

            if ((childPid == waitedChildPid) && (waitStatus == 0))
            {
                bRetValue = true;
            }
        }
    }

    return bRetValue;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    bool bRetValue = false;     // Assume error state at first.
    printf("Debug only: do_exec_redirect(): The outputfile is %s\n", outputfile);
    printf("Debug only: do_exec_redirect(): The count is %d\n", count);

    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
        // Debug only
        printf("Debug only: do_exec_redirect(): The command%d is %s\n", i, command[i]);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];


/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
 *      int execv(const char *path, char *const argv[]);
 *          RETURN VALUE
 *               A successful call to execv does not have a return value because the new process image overlays the calling process image. 
 *               However, a -1 is returned if the call to execv is unsuccessful.
*/

    va_end(args);

    bRetValue = true;   // TEMPORARY only!!!!
    return bRetValue;
}
