#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "functions.h"

#define SH_MEMORY_NAME "/air_traffic_shm"
#define TOTAL_TAKEOFFS 50
#define PLANES_LIMIT 20

int planes = 0;
int takeoffs = 0;
int total_takeoffs = 0;

pthread_mutex_t state_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t runway1_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t runway2_lock = PTHREAD_MUTEX_INITIALIZER;

int *shared_memory = NULL;
pid_t radio_pid = 0;

int main()
{
  // TODO 1: Call the function that creates the shared memory segment.
  MemoryCreate();

  // TODO 3: Configure the SIGUSR2 signal to increment the planes on the runway
  // by 5.
  struct sigaction sa;
  sa.sa_handler = SigHandler2;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  if (sigaction(SIGUSR2, &sa, NULL) == -1)
  {
    perror("sigaction failed");
    exit(1);
  }
  // TODO 4: Launch the 'radio' executable and, once launched, store its PID in
  // the second position of the shared memory block.
  radio_pid = fork();
  if (radio_pid == -1)
  {
    perror("fork failed");
    exit(1);
  }
  else if (radio_pid == 0)
  {
    execl("../../radio/build/./radio", "radio", SH_MEMORY_NAME, NULL);
    perror("execl failed");
    exit(1);
  }
  else
  {
    shared_memory[1] = radio_pid;
    sleep(1);
  }

  // TODO 6: Launch 5 threads which will be the controllers; each thread will
  // execute the TakeOffsFunction().
  pthread_t controllers[5];
  for (int i = 0; i < 5; i++)
  {
    if (pthread_create(&controllers[i], NULL, TakeOffsFunction, NULL) != 0)
    {
      perror("pthread_create failed");
      exit(1);
    }
  }
  for (int i = 0; i < 5; i++)
  {
    pthread_join(controllers[i], NULL);
  }

  munmap(shared_memory, 3 * sizeof(int));
  shm_unlink(SH_MEMORY_NAME);
  return 0;
}