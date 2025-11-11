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
#include <unistd.h>

int planes = 0;
int takeoffs = 0;
int traffic = 0;

/* file descriptor for the shared memory, visible to signal handlers */
int shm_fd = -1;

void Traffic(int signum) {
  // TODO:
  // Calculate the number of waiting planes.
  // Check if there are 10 or more waiting planes to send a signal and increment
  // planes. Ensure signals are sent and planes are incremented only if the
  // total number of planes has not been exceeded.
  if (traffic >= 10 && planes < PLANES_LIMIT) {
    kill(0, SIGUSR1);
    planes++;
  }
}

/* SIGTERM handler: close shared memory and exit */
static void term_handler(int signum) {
  (void)signum;
  printf("finalization of operations...\n");
  if (shm_fd != -1) {
    close(shm_fd);
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
  const char* shm_name = "/my_shm";
  shm_fd = shm_open(shm_name, O_RDWR | O_CREAT, 0666);
  if (shm_fd == -1) {
    perror("shm_open");
    exit(1);
  }

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
  printf("Ground control running (PID %d)\n", getpid());
  while (1) {
    pause();  // Wait for signals
  }
  return 0;
}