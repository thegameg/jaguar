#include <assert.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

typedef void *(*function_t)();

struct async_function
{
  function_t f;
  size_t nb_args;
  void *args[0];
};

// Function called in a separate thread.
void *tc_async_wrapper(void *arg)
{
  struct async *a = arg;
  for (ssize_t i = a->nb_args - 1; i >= 0; --i)
    __asm__ volatile ("push %%eax\n"
                     :
                     :"a"(a->args[i])
                     : "sp");

  void *res = (void *)a->f();

  __asm__ volatile ("add %0, %%esp\n"
                    :
                    :"r"(a->nb_args * sizeof (arg_t))
                    : "sp");
  free(a);

  return res;
}

/** \name Internal functions (calls generated by the compiler only). */
/** \{ */

/** \brief Call a function in a separate thread.
    \param f       The callee function.
    \param nb_args The number of arguments the function is taking.
    \param ...     The arguments to be passed to the function.

    An element size is always the size of a word on 32-bit systems.
*/
pthread_t tc_async_call(function_t f, int nb_args, ...)
{
  // Prepare the argument list.
  va_list ap;
  va_start(ap, nb_args);

  // Allocate heap space for the structure followed by the arguments.
  struct async_function *a = malloc(sizeof (struct async_function)
                                    + nb_args * sizeof (void *));
  a->nb_args = nb_args;
  a->f = f;

  // Fill the arguments.
  for (int i = 0; i < nb_args; ++i)
    a->args[i] = va_arg(ap, void *);

  // Cleanup the argument list.
  va_end(ap);

  // Create a thread with the intermediate function call.
  pthread_t thread;
  int res = pthread_create(&thread, NULL, &tc_async_wrapper, a);
  assert(!res && "Failed to create a thread.");
  return thread;
}
/** \} */

/** \name Internal functions (calls generated by the compiler only). */
/** \{ */

/** \brief Waits for the result of a task and sets the result.
    \param thread  The thread id that should be joined.
    \param result  The result memory slot.

    An element size is always the size of a word on 32-bit systems.
*/
void tc_async_return(pthread_t thread, void **result)
{
  pthread_join(thread, result);
}
/** \} */
