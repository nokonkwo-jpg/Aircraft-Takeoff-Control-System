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
#define PLANES_LIMIT 20

int planes = 0;
int takeoffs = 0;
int total_takeoffs = 0;

pthread_mutex_t state_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t runway1_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t runway2_lock = PTHREAD_MUTEX_INITIALIZER;

int *shared_memory = NULL;
pid_t radio_pid = 0;
pid_t ground_pid = 0;

int main()
{
  MemoryCreate();
  
  struct sigaction sa;
  sa.sa_handler = SigHandler2;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGUSR2, &sa, NULL);

  radio_pid = fork();
  if (radio_pid == 0) {
    execl("../../radio/build/radio", "radio", SH_MEMORY_NAME, NULL);
    perror("execl radio failed");
    exit(1);
  }
  shared_memory[1] = radio_pid;
  sleep(1);

  ground_pid = fork();
  if (ground_pid == 0) {
    execl("../../ground_control/build/ground_control_c", "ground_control_c", SH_MEMORY_NAME, NULL);
    perror("execl ground_control failed");
    exit(1);
  }
  sleep(1); 

  // Create 5 controller threads
  pthread_t controllers[5];
  for (int i = 0; i < 5; i++) {
    pthread_create(&controllers[i], NULL, TakeOffsFunction, NULL);
  }
  
  for (int i = 0; i < 5; i++) {
    pthread_join(controllers[i], NULL);
  }
  
  // Send SIGTERM to radio and ground_control
  kill(radio_pid, SIGTERM);
  kill(ground_pid, SIGTERM);
  sleep(1);
  
  munmap(shared_memory, 3 * sizeof(int));
  shm_unlink(SH_MEMORY_NAME);
  return 0;
}