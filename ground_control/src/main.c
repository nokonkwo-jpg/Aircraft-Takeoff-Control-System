#define PLANES_LIMIT 20
/* Request POSIX APIs such as sigaction from the C library */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>

int planes = 0;
int takeoffs = 0;
int traffic = 0;

/* file descriptor for the shared memory, visible to signal handlers */
int shm_fd = -1;
int *shm_ptr = NULL; /* mapped pointer to shared memory (3 ints) */

void Traffic(int signum) {
  // TODO:
  // Calculate the number of waiting planes.
  // Check if there are 10 or more waiting planes to send a signal and increment
  // planes. Ensure signals are sent and planes are incremented only if the
  // total number of planes has not been exceeded.
(void)signum;
  /* Calculate waiting planes: planes available minus takeoffs cleared */
  int waiting = planes - takeoffs;
  traffic = waiting;

  if (waiting >= 10) {
    printf("RUNWAY OVERLOADED\n");
  }

  /* If overall planes are below the limit, add 5 and notify radio */
  if (planes < PLANES_LIMIT) {
    planes += 5;
    printf("Ground: added 5 planes -> planes=%d\n", planes);
    if (shm_ptr != NULL) {
      pid_t radio_pid = (pid_t)shm_ptr[1];
      if (radio_pid > 0) {
        if (kill(radio_pid, SIGUSR2) == -1) {
          perror("Ground: failed to signal radio");
        } else {
          printf("Ground: signaled radio (pid=%d) with SIGUSR2\n", radio_pid);
        }
      }
    }
  }
}

/* SIGTERM handler: close shared memory and exit */
static void term_handler(int signum) {
  (void)signum;
  printf("finalization of operations...\n");
  /* Close and unmap shared memory before exiting */
  if (shm_ptr != NULL && shm_ptr != MAP_FAILED) {
    munmap(shm_ptr, 3 * sizeof(int));
    shm_ptr = NULL;
  }
  if (shm_fd != -1) {
    close(shm_fd);
    shm_fd = -1;
  }
  exit(0);
}

/* SIGUSR1 handler: increase number of takeoffs by 5 */
static void usr1_handler(int signum) {
  (void)signum;
  takeoffs += 5;
}

int main(int argc, char* argv[]) {
  // TODO:
  // 1. Open the shared memory block and store this process PID in position 2
  //    of the memory block.
  const char* shm_name = "/air_traffic_shm";
  /* Open existing shared memory created by the air control process */
  shm_fd = shm_open(shm_name, O_RDWR, 0666);
  if (shm_fd == -1) {
    perror("shm_open");
    exit(1);
  }

  /* Map three integers: [air_pid, radio_pid, ground_pid] */
  shm_ptr = mmap(NULL, 3 * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  if (shm_ptr == MAP_FAILED) {
    perror("mmap");
    close(shm_fd);
    exit(1);
  }

  /* Store our PID in position 2 */
  shm_ptr[2] = getpid();

  // 2. Configure SIGTERM and SIGUSR1 handlers
  //    - The SIGTERM handler should: close the shared memory, print
  //      "finalization of operations..." and terminate the program.
  //    - The SIGUSR1 handler should: increase the number of takeoffs by 5.

  struct sigaction sa_term;
  memset(&sa_term, 0, sizeof(sa_term));
  sa_term.sa_handler = term_handler;
  sigemptyset(&sa_term.sa_mask);
  sa_term.sa_flags = 0;
  sigaction(SIGTERM, &sa_term, NULL);

  // 3. Configure the timer to execute the Traffic function.
  struct sigaction sa_usr1;
  memset(&sa_usr1, 0, sizeof(sa_usr1));
  sa_usr1.sa_handler = usr1_handler;
  sigemptyset(&sa_usr1.sa_mask);
  sa_usr1.sa_flags = 0;
  sigaction(SIGUSR1, &sa_usr1, NULL);
  
  /* Configure SIGALRM to call Traffic every 500ms */
  struct sigaction sa_alrm;
  memset(&sa_alrm, 0, sizeof(sa_alrm));
  sa_alrm.sa_handler = Traffic;
  sigemptyset(&sa_alrm.sa_mask);
  sa_alrm.sa_flags = 0;
  sigaction(SIGALRM, &sa_alrm, NULL);

  struct itimerval timer;
  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = 500000; /* 500 ms */
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 500000; /* 500 ms */
  if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
    perror("setitimer");
    if (shm_ptr && shm_ptr != MAP_FAILED) munmap(shm_ptr, 3 * sizeof(int));
    close(shm_fd);
    exit(1);
  }
  printf("Ground control running (PID %d)\n", getpid());
  while (1) {
    pause();  // Wait for signals
  }
  return 0;
}