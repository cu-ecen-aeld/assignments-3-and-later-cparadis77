#include "threading.h"
#include <unistd.h>             // Used for sleep,  prototype: unsigned int sleep(unsigned int seconds);
                                //                                      int usleep(useconds_t usec);
                                //                                          -got 0 on success
                                //                                          -got -1 on failure

#include <stdlib.h>             // Used for malloc, prototype:  void *malloc(size_t size);
                                //                              void free(void *ptr);
#include <stdio.h>


// Optional: use these functions to add debug or error prints to your application
//#define DEBUG_LOG(msg,...)
#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)


void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    if (thread_func_args != NULL)
    {
        DEBUG_LOG("threadfunc(), thread started...");
        
        int retCode = usleep(thread_func_args->wait_to_obtain_ms * 1000);

        if (retCode == 0)
        {
            DEBUG_LOG("threadfunc(), mutex trying to lock...");
            retCode = pthread_mutex_lock(thread_func_args->pMutex);

            if (retCode == 0)
            {
                retCode = usleep(thread_func_args->wait_to_release_ms * 1000);

                if (retCode == 0)
                {
                    DEBUG_LOG("threadfunc(), mutex trying to unlock...");
                    retCode = pthread_mutex_unlock(thread_func_args->pMutex);

                    if (retCode == 0)
                    {
                        thread_func_args->thread_complete_success = true;
                    }
                    else
                    {
                        ERROR_LOG("threadfunc(), failed to unlock mutex, got retCode=%d", retCode);
                    }
                }
                else
                {
                    ERROR_LOG("threadfunc(), failed to sleep after lock, got retCode=%d", retCode);
                }
            }
            else
            {
                ERROR_LOG("threadfunc(), failed to lock mutex, got retCode=%d", retCode);
            } 
        }
        else
        {
            ERROR_LOG("threadfunc(), failed to sleep before lock, got retCode=%d", retCode);
        }
    }
    else
    {
        ERROR_LOG("threadfunc(), thread parameter is NULL");
    }

    DEBUG_LOG("threadfunc(), thread exit...");

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    bool bThreadStarted = false;

    DEBUG_LOG("start_thread_obtaining_mutex(), main function started... bThreadStarted:%d, wait_to_obtain_ms:%d, wait_to_release_ms:%d", bThreadStarted, wait_to_obtain_ms, wait_to_release_ms);

    // Check input args
    if (thread != NULL && mutex != NULL)
    {
        /**
         * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
         * using threadfunc() as entry point.
         *
         * return true if successful.
         *
         * See implementation details in threading.h file comment block
         */

        struct thread_data *pThreadData = NULL;
        pThreadData = (struct thread_data *)malloc(sizeof(struct thread_data));
        
        if (pThreadData != NULL)
        {
            // Note: Assuming that the passed in mutex has been initialised!! (This shall not be done twice!!)

            // Init structure data
            pThreadData->wait_to_obtain_ms = wait_to_obtain_ms;
            pThreadData->wait_to_release_ms = wait_to_release_ms;
            pThreadData->pMutex = mutex;
            pThreadData->thread_complete_success = false;

            //DEBUG_LOG("start_thread_obtaining_mutex(), before starting thread, threadId is: %s\n", thread-> );

            // Create thread, When thread exit it will return the thread_data with updated value
            int retCode = pthread_create(thread, NULL, threadfunc, (void *)pThreadData);

            if (retCode == 0)
            {
                bThreadStarted = true;

/*              //
                // Note: Commenting out since the implementation shall not block and wait for the thread to complete!!
                //

                struct thread_data *pThreadDataResult = NULL;

                // Thread has been created, wait for thread to complete
                retCode = pthread_join(*(thread), (void*)&pThreadDataResult);

                if (retCode == 0)
                {
                    if (pThreadDataResult != NULL && pThreadDataResult->thread_complete_success)
                    {
                        DEBUG_LOG("start_thread_obtaining_mutex(), thread_complete_success:%d", pThreadDataResult->thread_complete_success);
                        //bRetValue = true;
                    }
                }
                else
                {
                    // Unable to join thread
                    ERROR_LOG("start_thread_obtaining_mutex(), failed to join thread, got retCode=%d", retCode);
                }
*/                
            }
            else
            {
                // Unable to create thread
                ERROR_LOG("start_thread_obtaining_mutex(), failed to create thread, got retCode=%d", retCode);
            }

            // Clean-up, free allocated memory..........Commentting out since this is done in the Assignment AutoTest Framework!!!
            //DEBUG_LOG("start_thread_obtaining_mutex(), Clean-up, free allocated memory...bThreadStarted:%d", bThreadStarted);
            //free(pThreadData);
        }
        else
        {
            // Failed to alloc ThreadData structure
            ERROR_LOG("start_thread_obtaining_mutex(), failed to alloc thread_data structure");
        }
    }
    else
    {
        // thread or mutex is NULL
        ERROR_LOG("start_thread_obtaining_mutex(), at least 1 of thread parameter is NULL");
    }

    DEBUG_LOG("start_thread_obtaining_mutex(), main function exit...bThreadStarted:%d", bThreadStarted);

    return bThreadStarted;
}
