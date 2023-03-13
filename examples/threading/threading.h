#include <stdbool.h>
#include <pthread.h>            // Used for PThread API
                                //
                                // int pthread_mutex_init(pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *restrict attr);
                                // int pthread_mutex_destroy(pthread_mutex_t *mutex);
                                //  Note: It shall be safe to destroy an initialized mutex that is unlocked.
                                //          Attempting to destroy a locked mutex results in undefined behavior.
                                //  Note: If successful, the pthread_mutex_destroy() and pthread_mutex_init() functions shall return zero;
                                //          otherwise, an error number shall be returned to indicate the error.
                                //
                                // int pthread_mutex_lock(pthread_mutex_t *mutex);
                                // int pthread_mutex_unlock(pthread_mutex_t *mutex);
                                //  Note: If successful, the pthread_mutex_lock() and pthread_mutex_unlock() functions shall return zero;
                                //          otherwise, an error number shall be returned to indicate the error.
                                //
                                // int pthread_create(  pthread_t *restrict thread,
                                //                      const pthread_attr_t *restrict attr,
                                //                      void *(*start_routine)(void *),
                                //                      void *restrict arg);
                                //  Note: On success, pthread_create() returns 0; on error, it returns an
                                //          error number, and the contents of *thread are undefined.
                                //
                                // int pthread_join(pthread_t thread, void **retval);
                                //  Note:  On success, pthread_join() returns 0; on error, it returns an error number.
                                //  Note:  If that thread has already terminated, then pthread_join() returns immediately.
                                //  Note:  The thread specified by thread must be joinable.
                                //  Note:  If retval is not NULL, then pthread_join() copies the exit status of the target thread

                                
/**
 * This structure should be dynamically allocated and passed as
 * an argument to your thread using pthread_create.
 * It should be returned by your thread so it can be freed by
 * the joiner thread.
 */
struct thread_data{
    /*
     * TODO: add other values your thread will need to manage
     * into this structure, use this structure to communicate
     * between the start_thread_obtaining_mutex() function and
     * your thread implementation.
     */

    int wait_to_obtain_ms;
    int wait_to_release_ms;
    pthread_mutex_t *pMutex;

    /**
     * Set to true if the thread completed with success, false
     * if an error occurred.
     */
    bool thread_complete_success;
};


/**
* Start a thread which sleeps @param wait_to_obtain_ms number of milliseconds, then obtains the
* mutex in @param mutex, then holds for @param wait_to_release_ms milliseconds, then releases.
*
* The start_thread_obtaining_mutex() function should only start the thread and should not block
* for the thread to complete.
*
* The start_thread_obtaining_mutex() function should use dynamic memory allocation for thread_data
* structure passed into the thread.  The number of threads active should be limited only by the
* amount of available memory.
*
* The thread started should return a pointer to the thread_data structure when it exits, which can be used
* to free memory as well as to check thread_complete_success for successful exit.
*
* If a thread was started succesfully @param thread should be filled with the pthread_create thread ID
* coresponding to the thread which was started.
*
* @return true if the thread could be started, false if a failure occurred.
*/
bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex, int wait_to_obtain_ms, int wait_to_release_ms);
