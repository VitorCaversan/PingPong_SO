#include "ppos_disk.h"

/////////////////// STATIC VARIABLES DECLARATIONS ////////////////////

static ST_RequestList *gpstRequestList;
static disk_t disk;

static struct sigaction disk_action;

/////////////////// STATIC FUNCTIONS DECLARATIONS /////////////////////

/**
 * @brief Creates a Node object and returns it's pointer
 *
 * @param pstPreviousNode Pointer to it's previous node
 * @param pstNextNode     Pointer to it's next Node
 * @param pstTask         Pointer to the task that requested the action
 * @param block           Block number where the action will be done
 * @param buffer          Buffer where the data will be stored/read
 * @param uiStartingTick  Tick for when the action was requested
 * @return ST_RequestNode* Pointer to the new node
 */
static ST_RequestNode *createNode(ST_RequestNode *pstPreviousNode,
                                  ST_RequestNode *pstNextNode,
                                  task_t *pstTask,
                                  int block,
                                  void *buffer,
                                  char cTaskAction,
                                  unsigned int uiStartingTick);

/**
 * @brief Body for the disk task to run
 *
 * It calls the diskScheduler function constantly
 */
static void diskTaskBody();

/**
 * @brief Schedules the next disk block to be used, keeping track at the requisitions
 */
static int diskScheduler();

/**
 * @brief Function called when the SIGUSR1 signal is fired
 */
static void memActionFinished();

//////////////// EXTERNABLE FUNCTIONS DESCRIPTIONS ///////////////

// operações oferecidas pelo disco
// #define DISK_CMD_INIT		0	// inicializacao do disco
// #define DISK_CMD_READ		1	// leitura de bloco do disco
// #define DISK_CMD_WRITE		2	// escrita de bloco do disco
// #define DISK_CMD_STATUS		3	// consulta status do disco
// #define DISK_CMD_DISKSIZE	4	// consulta tamanho do disco em blocos
// #define DISK_CMD_BLOCKSIZE	5	// consulta tamanho de bloco em bytes
// #define DISK_CMD_DELAYMIN	6	// consulta tempo resposta mínimo (ms)
// #define DISK_CMD_DELAYMAX	7	// consulta tempo resposta máximo (ms)

extern int disk_mgr_init(int *numBlocks, int *blockSize)
{
   if (disk_cmd(DISK_CMD_INIT, 0, 0))
   {
      printf("Disk initialization error\n");
      return -1;
   }

   disk.size = disk_cmd(DISK_CMD_DISKSIZE, 0, 0);
   if (disk.size < 0)
   {
      printf("Disk size not found\n");
      return -1;
   }

   disk.iBlockSize = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0);
   if (disk.iBlockSize < 0)
   {
      printf("Block size not found\n");
      return -1;
   }

   *numBlocks = disk.size;
   *blockSize = disk.iBlockSize;

   disk.init = 1;

   mutex_create(&disk.mRequest);
   mutex_create(&disk.queueMutex);
   sem_create(&disk.treatedReqSem, 0);
   sem_create(&disk.newReqsSem, 0);

   disk.totalBlockAccess = 0;
   disk.execTime = 0;

   disk.packageSync = 0;

   gpstRequestList = createList();

   disk_action.sa_handler = memActionFinished;
   sigemptyset(&disk_action.sa_mask);
   disk_action.sa_flags = 0;

   if (sigaction(SIGUSR1, &disk_action, 0) < 0)
   {
      printf("Erro em sigaction");
      return -1;
   }

   task_create(&disk.task, diskTaskBody, NULL);
   task_setprio(&disk.task, 0);

   return 0;
}

extern int disk_block_read(int block, void *buffer)
{
   addNodeInFront(gpstRequestList, taskExec, block, buffer, DISK_CMD_READ, systemTime);

   mutex_lock(&disk.mRequest);

   // if (disk.packageSync > 0)
   // {
   //    disk.packageSync--;
   // }
   // else
   // {
   //    sem_up(&disk.newReqsSem);
   //    sem_down(&disk.treatedReqSem);
   // }

   sem_up(&disk.newReqsSem);
   sem_down(&disk.treatedReqSem);

   // if (0 == disk.packageSync)
   // {
   //    sem_up(&disk.newReqsSem);
   //    sem_down(&disk.treatedReqSem);
   // }
   // disk.packageSync--;

   mutex_unlock(&disk.mRequest);

   return 0;
}

extern int disk_block_write(int block, void *buffer)
{
   addNodeInFront(gpstRequestList, taskExec, block, buffer, DISK_CMD_WRITE, systemTime);

   mutex_lock(&disk.mRequest);

   // if (disk.packageSync > 0)
   // {
   //    disk.packageSync--;
   // }
   // else
   // {
   //    sem_up(&disk.newReqsSem);
   //    sem_down(&disk.treatedReqSem);
   // }

   sem_up(&disk.newReqsSem);
   sem_down(&disk.treatedReqSem);

   // if (0 == disk.packageSync)
   // {
   //    sem_up(&disk.newReqsSem);
   //    sem_down(&disk.treatedReqSem);
   // }
   // disk.packageSync = 0;

   mutex_unlock(&disk.mRequest);

   return 0;
}

extern void memActionFinished()
{
   disk.packageSync++;
   sem_up(&disk.treatedReqSem);
   disk.execTime += systemTime - disk.startingTime;

   return;
}

static void diskTaskBody()
{
   while (disk.init == 1)
   {
      sem_down(&disk.newReqsSem);
      diskScheduler();
   }
   
   printf("Number of accessed blocks %d\nExecution time in disk %d ms\n",
          disk.totalBlockAccess, disk.execTime);
   task_exit(0);
}

///////////////// STATIC FUNCTIONS DESCRIPTIONS /////////////////

ST_RequestNode *fcfsSched()
{
   return gpstRequestList->firstNode;
}

ST_RequestNode *sstfSched()
{
   int iSize = gpstRequestList->iSize;
   ST_RequestNode *pstIter = gpstRequestList->firstNode;

   int headBlock = disk.iCurrBlock;
   int shortest = abs(pstIter->block - headBlock);
   ST_RequestNode *pstNearestReq = pstIter;

   int seekDist = 0;
   int i = 0;
   for (i = 0; i < iSize && pstIter != NULL; i++, pstIter = pstIter->next)
   {
      seekDist = abs(pstIter->block - headBlock);
      if (seekDist < shortest)
      {
         shortest = seekDist;
         pstNearestReq = pstIter;
      }
   }

   return pstNearestReq;
}

ST_RequestNode *cscanSched()
{
   int iSize = gpstRequestList->iSize;
   ST_RequestNode *pstIter = gpstRequestList->firstNode;

   int head = disk.iCurrBlock;
   int iShortestFront = pstIter->block - head;
   int iShortest = pstIter->block;

   ST_RequestNode *pstShortestReq = pstIter;
   ST_RequestNode *pstShortestReq2 = pstIter;

   int iBlockDist = 0;
   int i = 0;
   char cIsInFront = 0;
   for (i = 0; i < iSize && pstIter != NULL; i++, pstIter = pstIter->next)
   {
      iBlockDist = pstIter->block - head;
      if (iBlockDist >= 0)
      {
         cIsInFront = 1;
         if (iBlockDist < iShortestFront)
         {
            iShortestFront = iBlockDist;
            pstShortestReq = pstIter;
         }
      }
      else
      {
         iBlockDist = pstIter->block;
         if (iBlockDist < iShortest)
         {
            iShortest = iBlockDist;
            pstShortestReq2 = pstIter;
         }
      }
   }

   if (cIsInFront)
   {
      return pstShortestReq;
   }
   else
   {
      return pstShortestReq2;
   }
}

static int diskScheduler()
{
   if (isEmpty(gpstRequestList))
   {
      return 0;
   }

   ST_RequestNode *pstNextReq;
   disk.enAlgorithm = SSTF;
   switch (disk.enAlgorithm)
   {
   case FCFS:
      pstNextReq = fcfsSched();
      break;

   case SSTF:
      pstNextReq = sstfSched();
      break;

   case CSCAN:
      pstNextReq = cscanSched();
      break;

   default:
      pstNextReq = fcfsSched();
   }

   removeNode(gpstRequestList, pstNextReq);

   disk.status = disk_cmd(DISK_CMD_STATUS, 0, 0);

   if (DISK_STATUS_IDLE != disk.status)
   {
      printf("disk_status = %d\n", disk.status);
      return -1;
   }

   disk.startingTime = systemTime;
   disk.totalBlockAccess += abs(pstNextReq->block - disk.iCurrBlock);
   disk.iCurrBlock = pstNextReq->block;
   disk.buffer = pstNextReq->buffer;

   if (disk_cmd(pstNextReq->cTaskAction, disk.iCurrBlock, disk.buffer) != 0)
   {
      printf("Falha ao ler/escrever o bloco %d %p %d %d\n", disk.iCurrBlock, disk.buffer, disk.size, disk.status);
      sem_up(&disk.treatedReqSem);
      return -1;
   }

   return 0;
}

static ST_RequestNode *createNode(ST_RequestNode *pstPreviousNode,
                                  ST_RequestNode *pstNextNode,
                                  task_t *pstTask,
                                  int block,
                                  void *buffer,
                                  char cTaskAction,
                                  unsigned int uiStartingTick)
{
   ST_RequestNode *pstNewNode = (ST_RequestNode *)malloc(sizeof(ST_RequestNode));

   pstNewNode->prev = pstPreviousNode;
   pstNewNode->next = pstNextNode;
   pstNewNode->task = pstTask;
   pstNewNode->block = block;
   pstNewNode->buffer = buffer;
   pstNewNode->cTaskAction = cTaskAction;
   pstNewNode->startingTime = uiStartingTick;

   return pstNewNode;
}

//////////// DOUBLE LINKED LIST FUNCTIONS /////////////

extern ST_RequestList *createList()
{
   // printf("LIST TO BE CREATED\n");
   ST_RequestList *pstNewList = (ST_RequestList *)malloc(sizeof(ST_RequestList));
   pstNewList->firstNode = NULL;
   pstNewList->lastNode = NULL;
   pstNewList->iSize = 0;

   // printf("LIST CREATED\n");

   return pstNewList;
}

extern void addNode(ST_RequestList *pstList,
                    ST_RequestNode *pstPreviousNode,
                    task_t *pstTask,
                    int block,
                    int *buffer,
                    char cTaskAction,
                    unsigned int uiStartingTick)
{
   // printf("NODE TO BE ADDED\n");
   if (NULL != pstPreviousNode)
   {
      ST_RequestNode *pstNewNode = createNode(pstPreviousNode, pstPreviousNode->next, pstTask, block, buffer, cTaskAction, uiStartingTick);

      if (pstPreviousNode == pstList->lastNode)
      {
         pstList->lastNode = pstNewNode;
      }
   }
   else
   {
      ST_RequestNode *pstNewNode = createNode(NULL, NULL, pstTask, block, buffer, cTaskAction, uiStartingTick);

      pstList->firstNode = pstNewNode;
      pstList->lastNode = pstNewNode;
   }

   (pstList->iSize)++;

   // printf("NODE ADDED\n");

   return;
}

extern void addNodeInFront(ST_RequestList *pstList,
                           task_t *pstTask,
                           int block,
                           void *buffer,
                           char cTaskAction,
                           unsigned int uiStartingTick)
{
   // printf("NODE TO BE ADDED IN FRONT\n");
   if (NULL == pstList)
   {
      return;
   }

   mutex_lock(&disk.queueMutex);
   if (NULL == pstList->firstNode)
   {
      ST_RequestNode *pstNewNode = createNode(NULL, NULL, pstTask, block, buffer, cTaskAction, uiStartingTick);
      pstList->firstNode = pstNewNode;
      pstList->lastNode = pstList->firstNode;
   }
   else
   {
      ST_RequestNode *pstNewNode = createNode(pstList->lastNode, NULL, pstTask, block, buffer, cTaskAction, uiStartingTick);
      pstList->lastNode->next = pstNewNode;
      pstList->lastNode = pstNewNode;
   }
   (pstList->iSize)++;
   mutex_unlock(&disk.queueMutex);
   return;
}

extern void addNodeInBack(ST_RequestList *pstList,
                          task_t *pstTask,
                          int block,
                          int *buffer,
                          char cTaskAction,
                          unsigned int uiStartingTick)
{
   // printf("NODE TO BE ADDED IN BACK\n");
   if (NULL == pstList)
   {
      return;
   }

   if (NULL == pstList->firstNode)
   {
      pstList->firstNode = createNode(NULL, NULL, pstTask, block, buffer, cTaskAction, uiStartingTick);
      pstList->lastNode = pstList->firstNode;
   }
   else
   {
      ST_RequestNode *pstNewNode = createNode(NULL, pstList->firstNode, pstTask, block, buffer, cTaskAction, uiStartingTick);
      pstList->firstNode->prev = pstNewNode;
      pstList->firstNode = pstNewNode;
   }

   (pstList->iSize)++;

   // printf("NODE ADDED IN BACK\n");

   return;
}

extern void removeNode(ST_RequestList *pstList, ST_RequestNode *pstNode)
{
   // printf("NODE TO BE REMOVED\n");
   if ((NULL == pstList) || (NULL == pstNode))
   {
      return;
   }

   if ((NULL == pstNode->next) && (NULL == pstNode->prev))
   {
      pstList->firstNode = NULL;
      pstList->lastNode = NULL;
   }
   else if (NULL == pstNode->next)
   {
      pstNode->prev->next = NULL;
      pstList->lastNode = pstNode->prev;
   }
   else if (NULL == pstNode->prev)
   {
      pstNode->next->prev = NULL;
      pstList->firstNode = pstNode->next;
   }
   else
   {
      pstNode->prev->next = pstNode->next;
      pstNode->next->prev = pstNode->prev;
   }

   free(pstNode);

   (pstList->iSize)--;
   // printf("NODE REMOVED, list size: %d\n", pstList->iSize);

   return;
}

extern char finishDiskTask()
{
   if ((NULL != gpstRequestList) && (gpstRequestList->iSize != 0))
   {
      ST_RequestNode *aux  = gpstRequestList->firstNode;
      ST_RequestNode *aux2 = gpstRequestList->firstNode;

      while (aux != NULL)
      {
         aux2 = aux->next;
         free(aux);
         aux = aux2;
      }

      gpstRequestList->firstNode = NULL;
      gpstRequestList->lastNode  = NULL;
      gpstRequestList->iSize     = 0;
      disk.init = 0;
      sem_up(&disk.newReqsSem);

      printf("List freed successfully\n");

      return 1;
   }
   else
   {
      return 0;
   }
}

extern char isEmpty(ST_RequestList *pstList)
{
   return ((NULL == pstList) || (NULL == pstList->firstNode));
}