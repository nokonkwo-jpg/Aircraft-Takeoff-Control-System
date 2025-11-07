#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define TOTAL_TAKEOFFS 50
#define PLANES_LIMIT 20

int planes = 0;
int takeoffs = 0;
int total_takeoffs = 0;

int main() {
  // TODO 1: Call the function that creates the shared memory segment.

  // TODO 3: Configure the SIGUSR2 signal to increment the planes on the runway
  // by 5.

  // TODO 4: Launch the 'radio' executable and, once launched, store its PID in
  // the second position of the shared memory block.

  // TODO 6: Launch 5 threads which will be the controllers; each thread will
  // execute the TakeOffsFunction().
}