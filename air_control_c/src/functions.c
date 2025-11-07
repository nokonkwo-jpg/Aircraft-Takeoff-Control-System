#include <sys/mman.h>
#include <sys/stat.h>


#define SH_MEMORY_NAME "/air_traffic_shm"

int planes = 0;
int takeoffs = 0;
int total_takeoffs = 0;

void MemoryCreate() {
  // TODO2: create the shared memory segment, configure it and store the PID of
  // the process in the first position of the memory block.
  int shm_fd = shm_open(SH_MEMORY_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        exit(1);
    }
}

void SigHandler2(int signal) {
  printf("[DEBUG] SIGUSR2 received (not implemented yet)\n");
}

void* TakeOffsFunction() {
  // TODO: implement the logic to control a takeoff thread
  //    Use a loop that runs while total_takeoffs < TOTAL_TAKEOFFS
  //    Use runway1_lock or runway2_lock to simulate a locked runway
  //    Use state_lock for safe access to shared variables (planes,
  //    takeoffs, total_takeoffs)
  //    Simulate the time a takeoff takes with sleep(1)
  //    Send SIGUSR1 every 5 local takeoffs
  //    Send SIGTERM when the total takeoffs target is reached
  printf("[DEBUG] Thread started (not implemented yet)\n");
    return NULL;
}