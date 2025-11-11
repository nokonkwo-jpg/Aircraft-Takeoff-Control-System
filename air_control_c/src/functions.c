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

extern int planes;
extern int takeoffs;
extern int total_takeoffs;

extern pthread_mutex_t state_lock;
extern pthread_mutex_t runway1_lock;
extern pthread_mutex_t runway2_lock;

extern int *shared_memory;
extern pid_t radio_pid;

void MemoryCreate()
{
  // TODO2: create the shared memory segment, configure it and store the PID of
  // the process in the first position of the memory block.
  int shm_fd = shm_open(SH_MEMORY_NAME, O_CREAT | O_RDWR, 0666);
  if (shm_fd == -1)
  {
    perror("shm_open failed");
    exit(1);
  }

  if (ftruncate(shm_fd, 3 * sizeof(int)) == -1)
  {
    perror("ftruncate failed");
    exit(1);
  }

  shared_memory = mmap(NULL, 3 * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (shared_memory == MAP_FAILED)
  {
    perror("mmap failed");
    exit(1);
  }
  shared_memory[0] = getpid();
}

void SigHandler2(int signal)
{
  pthread_mutex_lock(&state_lock);
  planes += 5;
  pthread_mutex_unlock(&state_lock);
}

void *TakeOffsFunction(void *arg)
{
  // TODO: implement the logic to control a takeoff thread
  //    Use a loop that runs while total_takeoffs < TOTAL_TAKEOFFS
  //    Use runway1_lock or runway2_lock to simulate a locked runway
  //    Use state_lock for safe access to shared variables (planes,
  //    takeoffs, total_takeoffs)
  //    Simulate the time a takeoff takes with sleep(1)
  //    Send SIGUSR1 every 5 local takeoffs
  //    Send SIGTERM when the total takeoffs target is reached
  while (total_takeoffs < TOTAL_TAKEOFFS)
  {
    pthread_mutex_t *acquired_runway = NULL;

    while (acquired_runway == NULL)
    {
      if (pthread_mutex_trylock(&runway1_lock) == 0)
      {
        acquired_runway = &runway1_lock;
        break;
      }
      else if (pthread_mutex_trylock(&runway2_lock) == 0)
      {
        acquired_runway = &runway2_lock;
        break;
      }
      else
      {
        usleep(1000); // Sleep before retrying
      }
    }
    pthread_mutex_lock(&state_lock);
    if (total_takeoffs >= TOTAL_TAKEOFFS)
    {
      pthread_mutex_unlock(&state_lock);
      pthread_mutex_unlock(acquired_runway);
      break;
    }
    if (planes > 0)
    {
      planes--;
      takeoffs++;
      total_takeoffs++;
      if (takeoffs % 5 == 0)
      {
        kill(radio_pid, SIGUSR1);
        takeoffs = 0;
      }

      int should_terminate = (total_takeoffs >= TOTAL_TAKEOFFS);
      pthread_mutex_unlock(&state_lock);
      sleep(1); // Simulate takeoff time
      pthread_mutex_unlock(acquired_runway);
      if (should_terminate)
      {
        kill(radio_pid, SIGTERM);
        break;
      }
      else
      {
        pthread_mutex_unlock(&state_lock);
        pthread_mutex_unlock(acquired_runway);
        usleep(100000); // Sleep for 100ms before retrying
      }
    }
  }
  return NULL;
}