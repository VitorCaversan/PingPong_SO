#include "ppos.h"
#include "ppos-core-globals.h"
#include "ppos_disk.h"
#include <signal.h>
#include <sys/time.h>

#define UNIX_MAX_PRIO           20
#define UNIX_MIN_PRIO           -20
#define UNIX_AGING_FACTOR       -1

#define TIMER_STARTING_SHOT_S    1
#define TIMER_PERIOD_uS          1000
#define DEFAULT_TASK_TICKS       40

#define	SIGALRM                  14

// ****************************************************************************
// Coloque aqui as suas modificações, p.ex. includes, defines variáveis, 
// estruturas e funções

// STATIC VARIABLES DECLARATIONS ==============================================

// Structure to handle interruptions fired by the timer
static struct sigaction stAction;
// Handles signal fired by the mem task
static struct sigaction stMemAction;

// Timer used as a ticks simulation
static struct itimerval stTimer;

static unsigned int uiTaskStartingTick = 0;

// STATIC FUNCTIONS DECLARATIONS ==============================================

/**
 * @brief Gets the task with the highest priority in the list
 * 
 * @param pstFirstTask The start of the list
 * @return task_t* Pointer to the highest priority task
 */
static task_t *getHighestPrioTaks(task_t *pstFirstTask);

/**
 * @brief A handler for the implemented tick system
 * 
 * For each 20 ticks fired, this function calls scheduler() to decide the next
 * task to be executed
 * 
 * @param signum An ID for the interruption
 */
static void tickHandler(int signum);

/**
 * @brief Updates the tasks metric parameters when preempting
 * 
 * @param pstPreviousTask Pointer to the previous task
 * @param pstNextTask     Pointer to the next task
 */
static void metricsHandler(task_t *pstPreviousTask, task_t *pstNextTask);

/**
 * @brief A function that prints the information of the task being exited
 * 
 * It uses global variables and pointers to decide what informations from which tasks to print.
 */
static void printTaskInfo(void);

/**
 * @brief Handles the SIGUSR1 signal fired by the mem task
 * 
 * @param signum An ID for the signal
 */
static void memActionHandler(int signum);

// ****************************************************************************



void before_ppos_init () {
    // Interrupt handler initialization
    stAction.sa_handler = tickHandler;
    sigemptyset (&stAction.sa_mask);
    stAction.sa_flags = 0 ;

    if (sigaction(SIGALRM, &stAction, 0) < 0)
    {
        perror ("Sigaction error: ") ;
        exit (1) ;
    }

    // Timer initialization
    stTimer.it_value.tv_sec  = TIMER_STARTING_SHOT_S;
    stTimer.it_value.tv_usec = 0;
    stTimer.it_interval.tv_sec  = 0;
    stTimer.it_interval.tv_usec = TIMER_PERIOD_uS;

    if (setitimer(0, &stTimer, 0) < 0)
    {
        perror ("Setitimer error: ") ;
        exit (1) ;
    }

    // Interrupt handler initialization
    stMemAction.sa_handler = tickHandler;
    sigemptyset (&stMemAction.sa_mask);
    stMemAction.sa_flags = 0 ;

    if (sigaction(SIGUSR1, &stAction, 0) < 0)
    {
        perror ("Sigaction error: ") ;
        exit (1) ;
    }
    
#ifdef DEBUG
    printf("\ninit - BEFORE");
#endif
}

void after_ppos_init () {
    // put your customization here
#ifdef DEBUG
    printf("\ninit - AFTER");
#endif
}

void before_task_create (task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_create - BEFORE - [%d]", task->id);
#endif
}

void after_task_create (task_t *task ) {
    // put your customization here
    task->uiExecTicks = systemTime;
#ifdef DEBUG
    printf("\ntask_create - AFTER - [%d]", task->id);
#endif
}

void before_task_exit () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_exit - BEFORE - [%d]", taskExec->id);
#endif
}

void after_task_exit () {
    // put your customization here

    // Updates how many ticks the task has taken to be fully executed
    taskExec->uiExecTicks = (systemTime - taskExec->uiExecTicks);

    printTaskInfo();

#ifdef DEBUG
    printf("\ntask_exit - AFTER- [%d]", taskExec->id);
#endif
}

void before_task_switch ( task_t *task ) {
    // put your customization here
    metricsHandler(taskExec, task);
#ifdef DEBUG
    printf("\ntask_switch - BEFORE - [%d -> %d]", taskExec->id, task->id);
#endif
}

void after_task_switch ( task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_switch - AFTER - [%d -> %d]", taskExec->id, task->id);
#endif
}

void before_task_yield () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_yield - BEFORE - [%d]", taskExec->id);
#endif
}
void after_task_yield () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_yield - AFTER - [%d]", taskExec->id);
#endif
}


void before_task_suspend( task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_suspend - BEFORE - [%d]", task->id);
#endif
}

void after_task_suspend( task_t *task ) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_suspend - AFTER - [%d]", task->id);
#endif
}

void before_task_resume(task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_resume - BEFORE - [%d]", task->id);
#endif
}

void after_task_resume(task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_resume - AFTER - [%d]", task->id);
#endif
}

void before_task_sleep () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_sleep - BEFORE - [%d]", taskExec->id);
#endif
}

void after_task_sleep () {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_sleep - AFTER - [%d]", taskExec->id);
#endif
}

int before_task_join (task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_join - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_task_join (task_t *task) {
    // put your customization here
#ifdef DEBUG
    printf("\ntask_join - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}


int before_sem_create (semaphore_t *s, int value) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_create (semaphore_t *s, int value) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_down (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_down - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_down (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_down - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_up (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_up - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_up (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_up - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_sem_destroy (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_sem_destroy (semaphore_t *s) {
    // put your customization here
#ifdef DEBUG
    printf("\nsem_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_create (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_create (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_lock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_lock - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_lock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_lock - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_unlock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_unlock - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_unlock (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_unlock - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mutex_destroy (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mutex_destroy (mutex_t *m) {
    // put your customization here
#ifdef DEBUG
    printf("\nmutex_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_create (barrier_t *b, int N) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_create (barrier_t *b, int N) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_join (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_join - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_join (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_join - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_barrier_destroy (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_barrier_destroy (barrier_t *b) {
    // put your customization here
#ifdef DEBUG
    printf("\nbarrier_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_create (mqueue_t *queue, int max, int size) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_create - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_create (mqueue_t *queue, int max, int size) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_create - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_send (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_send - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_send (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_send - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_recv (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_recv - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_recv (mqueue_t *queue, void *msg) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_recv - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_destroy (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_destroy - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_destroy (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_destroy - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

int before_mqueue_msgs (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_msgs - BEFORE - [%d]", taskExec->id);
#endif
    return 0;
}

int after_mqueue_msgs (mqueue_t *queue) {
    // put your customization here
#ifdef DEBUG
    printf("\nmqueue_msgs - AFTER - [%d]", taskExec->id);
#endif
    return 0;
}

void task_setprio (task_t *task, int prio)
{
    if ((UNIX_MIN_PRIO <= prio) && (UNIX_MAX_PRIO >= prio))
    {
        if (NULL != task)
        {
            task->iStaticPrio = prio;
        }
        else
        {
            taskExec->iStaticPrio = prio;
        }
    }

    return;
}

int task_getprio (task_t *task)
{
    int iPrio = 0;

    if (NULL != task)
    {
        iPrio = task->iStaticPrio;
    }
    else
    {
        iPrio = taskExec->iStaticPrio;
    }

    return iPrio;
}

task_t *scheduler()
{
    task_t *pstNextTask = readyQueue;

    if (NULL != readyQueue)
    {
        pstNextTask = getHighestPrioTaks(readyQueue);
        pstNextTask->iDinamPrio = pstNextTask->iStaticPrio;

        task_t *pstListRunner = readyQueue;
        task_t *pstFirstTask  = readyQueue;

        // The tasks list is a circular list, so this is the way to run through it
        do
        {
            if (pstListRunner != pstNextTask)
            {
                pstListRunner->iDinamPrio += UNIX_AGING_FACTOR;
            }

            pstListRunner = pstListRunner->next;
        }
        while (pstFirstTask != pstListRunner);
    }

    return pstNextTask;
}

#if 0
unsigned int systime()
{
    return (unsigned int)systemTime;
}
#endif


// STATIC FUNCTIONS DEFINITIONS ================================================

static task_t *getHighestPrioTaks(task_t *pstFirstTask)
{
    task_t *pstHighestTask = pstFirstTask;
    task_t *pstListRunner  = pstFirstTask;

    do
    {
        if (pstListRunner->iDinamPrio < pstHighestTask->iDinamPrio)
        {
            pstHighestTask = pstListRunner;
        }

        pstListRunner = pstListRunner->next;
    }
    while (pstFirstTask != pstListRunner);
    
    return pstHighestTask;
}

static void tickHandler(int signum)
{
    static int iTaskTicksQty = DEFAULT_TASK_TICKS;

    iTaskTicksQty--;
    systemTime++;

    if (0 >= iTaskTicksQty)
    {
        iTaskTicksQty = DEFAULT_TASK_TICKS;

        if (taskExec != taskDisp)
        {
            task_yield();
        }
    }

    return;
}

static void metricsHandler(task_t *pstPreviousTask, task_t *pstNextTask)
{
    (pstPreviousTask->uiProcessorTicks) += (systemTime - uiTaskStartingTick);

    (pstNextTask->uiActivations)++;

    uiTaskStartingTick = systemTime;

    return;
}

static void printTaskInfo(void)
{
    printf("Task %d exit: Execution time: %d ms Processor time: %d ms %d activations",
            taskExec->id,
            taskExec->uiExecTicks,
            taskExec->uiProcessorTicks,
            taskExec->uiActivations);
    
    if (countTasks < 2)
    {
        printf("\nTask %d exit: Execution time: %d ms Processor time: %d ms %d activations\n",
               taskDisp->id,
               taskDisp->uiExecTicks,
               taskDisp->uiProcessorTicks,
               taskDisp->uiActivations);
        printf("Task %d exit: Execution time: %d ms Processor time: %d ms %d activations\n",
               taskMain->id,
               taskMain->uiExecTicks,
               taskMain->uiProcessorTicks,
               taskMain->uiActivations);
    }

    return;
}

static void memActionHandler(int signum)
{
    memActionFinished();

    return;
}