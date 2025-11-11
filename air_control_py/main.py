import ctypes
import mmap
import os
import signal
import subprocess
import threading
import time

_libc = ctypes.CDLL(None, use_errno=True)

TOTAL_TAKEOFFS = 20
STRIPS = 5
shm_data = []

# TODO1: Size of shared memory for 3 integers (current process pid, radio, ground) use ctypes.sizeof()
SHM_LENGTH = 3 * ctypes.sizeof(ctypes.c_int)

# Global variables and locks
planes = 0  # planes waiting
takeoffs = 0  # local takeoffs (per thread)
total_takeoffs = 0  # total takeoffs

state_lock = threading.Lock()
runway1_lock = threading.Lock()
runway2_lock = threading.Lock()


def create_shared_memory():
    """Create shared memory segment for PID exchange"""
    # TODO 6:
    # 1. Encode (utf-8) the shared memory name to use with shm_open
    shm_name = b"/air_traffic_shm"
    
    # 2. Temporarily adjust the permission mask (umask) so the memory can be created with appropriate permissions
    old_umask = os.umask(0)
    
    # 3. Use _libc.shm_open to create the shared memory
    shm_fd = _libc.shm_open(shm_name, os.O_CREAT | os.O_RDWR, 0o666)
    if shm_fd < 0:
        raise OSError("shm_open failed")
    
    # 4. Use _libc.ftruncate to set the size of the shared memory (SHM_LENGTH)
    if _libc.ftruncate(shm_fd, SHM_LENGTH) != 0:
        raise OSError("ftruncate failed")
    
    # 5. Restore the original permission mask (umask)
    os.umask(old_umask)
    
    # 6. Use mmap to map the shared memory
    memory = mmap.mmap(shm_fd, SHM_LENGTH, mmap.MAP_SHARED, mmap.PROT_READ | mmap.PROT_WRITE)
    
    # 7. Create an integer-array view (use memoryview()) to access the shared memory
    data = memoryview(memory).cast('i')
    
    # Store current process PID in position 0
    data[0] = os.getpid()
    
    # 8. Return the file descriptor (shm_open), mmap object and memory view
    return shm_fd, memory, data


def HandleUSR2(signum, frame):
    """Handle external signal indicating arrival of 5 new planes.
    Complete function to update waiting planes"""
    global planes
    # TODO 4: increment the global variable planes
    with state_lock:
        planes += 5


def TakeOffFunction(agent_id: int):
    """Function executed by each THREAD to control takeoffs.
    Complete using runway1_lock and runway2_lock and state_lock to synchronize"""
    global planes, takeoffs, total_takeoffs

    # TODO: implement the logic to control a takeoff thread
    # Use a loop that runs while total_takeoffs < TOTAL_TAKEOFFS
    while total_takeoffs < TOTAL_TAKEOFFS:
        # Use runway1_lock or runway2_lock to simulate runway being locked
        acquired_runway = None
        
        while acquired_runway is None:
            if runway1_lock.acquire(blocking=False):
                acquired_runway = runway1_lock
                break
            if runway2_lock.acquire(blocking=False):
                acquired_runway = runway2_lock
                break
            time.sleep(0.001)
        
        # Use state_lock for safe access to shared variables (planes, takeoffs, total_takeoffs)
        with state_lock:
            if total_takeoffs >= TOTAL_TAKEOFFS:
                acquired_runway.release()
                break
            
            if planes > 0:
                planes -= 1
                takeoffs += 1
                total_takeoffs += 1
                
                # Send SIGUSR1 every 5 local takeoffs
                if takeoffs == 5:
                    os.kill(shm_data[1], signal.SIGUSR1)
                    takeoffs = 0
                
                should_terminate = (total_takeoffs >= TOTAL_TAKEOFFS)
            else:
                acquired_runway.release()
                time.sleep(0.001)
                continue
        
        # Simulate the time a takeoff takes with sleep(1)
        time.sleep(1)
        
        acquired_runway.release()
        
        # Send SIGTERM when the total takeoffs target is reached
        if should_terminate:
            os.kill(shm_data[1], signal.SIGTERM)
            break


def launch_radio():
    """unblock the SIGUSR2 signal so the child receives it"""
    def _unblock_sigusr2():
        signal.pthread_sigmask(signal.SIG_UNBLOCK, {signal.SIGUSR2})

    # TODO 8: Launch the external 'radio' process using subprocess.Popen()
    process = subprocess.Popen(
        ["../radio/build/./radio", "/air_traffic_shm"],
        preexec_fn=_unblock_sigusr2
    )
    return process


def main():
    global shm_data

    # TODO 2: set the handler for the SIGUSR2 signal to HandleUSR2
    signal.signal(signal.SIGUSR2, HandleUSR2)
    
    # TODO 5: Create the shared memory and store the current process PID using create_shared_memory()
    fd, memory, data = create_shared_memory()
    shm_data = data
    
    # TODO 7: Run radio and store its PID in shared memory, use the launch_radio function
    radio_process = launch_radio()
    shm_data[1] = radio_process.pid
    time.sleep(1)
    
    # TODO 9: Create and start takeoff controller threads (STRIPS)
    threads = []
    for i in range(STRIPS):
        thread = threading.Thread(target=TakeOffFunction, args=(i,))
        thread.start()
        threads.append(thread)
    
    # TODO 10: Wait for all threads to finish their work
    for thread in threads:
        thread.join()
    
    # TODO 11: Release shared memory and close resources
    memory.close()
    os.close(fd)
    _libc.shm_unlink(b"/air_traffic_shm")


if __name__ == "__main__":
    main()