# PingPong_SO
A simple operational system that implements a thread scaler and a disc manager.

# Core library
The base of the project was a library that implemented a virtual disk in a running application,
together with some basic task and disk management functions.

The library was provided by prof. Maziero, a Operational Systems professor from UFPR
(Universidade Federal do Paraná).

# Project parts
The project was divided in two parts, both of tem described below.

# Part A: Task scheduler implementation
This part consisted on the following tasks:
   - Implement a preemptive scaler, based on aging;
   - Implement a time based preemption;
   - Measure the task execution metrics.

Easy stuff. Look for the scheduler() function.

# Part B: Disk manager implementation
This part consisted on the following tasks:
   - Implement a virtual disk manager;
   - Implement a disk access request manager.

The group opted to use semaphores and mutexes to manage all disk access requests.

Each request is put into a double linked list and than a sem_up() is called on the
disk request semaphore and a sem_down() on the processed request semaphore.

For each request that begins treatment, a sem_down() is called on the
disk request semaphore. For each request that has finished treatment, a sem_up() is
called on the processed request semaphore.

To summarize, the semephores were used as signals for the continuation of other tasks.