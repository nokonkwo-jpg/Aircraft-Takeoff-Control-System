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
#define TOTAL_TAKEOFFS 20

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
  while (1)
  {
    pthread_mutex_t *acquired_runway = NULL;

    while (acquired_runway == NULL)
    {
      if (pthread_mutex_trylock(&runway1_lock) == 0)
      {
        acquired_runway = &runway1_lock;
      }
      else if (pthread_mutex_trylock(&runway2_lock) == 0)
      {
        acquired_runway = &runway2_lock;
      }
      else
      {
        usleep(1000);
      }
    }

    pthread_mutex_lock(&state_lock);
    
    // Check if we're done
    if (total_takeoffs >= TOTAL_TAKEOFFS)
    {
      pthread_mutex_unlock(&state_lock);
      pthread_mutex_unlock(acquired_runway);
      break;
    }

    // Check if there are planes available
    if (planes > 0)
    {
      // Perform takeoff
      planes--;
      takeoffs++;
      total_takeoffs++;

      // Check if we need to signal radio (every 5 takeoffs)
      if (takeoffs >= 5)
      {
        kill(radio_pid, SIGUSR1);
        takeoffs = 0;
      }

      pthread_mutex_unlock(&state_lock);
      
      // Simulate takeoff time
      sleep(1);
      
      // Release the runway
      pthread_mutex_unlock(acquired_runway);
    }
    else
    {
      pthread_mutex_unlock(&state_lock);
      pthread_mutex_unlock(acquired_runway);
      usleep(100000);
    }
  }
  
  return NULL;
}