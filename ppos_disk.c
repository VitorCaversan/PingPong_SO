#include "ppos_disk.h"

/////////////////// STATIC VARIABLES DECLARATIONS ////////////////////

static ST_RequestList *gpstRequestList;
static disk_t         disk;

static struct sigaction disk_action ;
int disk_wait;

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
                                  task_t         *pstTask,
                                  int            block,
                                  int            *buffer,
                                  char           cTaskAction,
                                  unsigned int   uiStartingTick);

/**
 * @brief Function called when the SIGUSR1 signal is fired
 */
static void memActionFinished();

static void diskTaskBody();
static int disk_scheduler();

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

extern int disk_mgr_init (int *numBlocks, int *blockSize)
{
   if(disk_cmd(DISK_CMD_INIT, 0, 0))
   {
      printf("Falha ao iniciar o disco\n");
      return -1;
   }

   disk.tam = disk_cmd(DISK_CMD_DISKSIZE, 0, 0);
   if(disk.tam < 0)
   {
      printf("Falha ao consultar o tamanho do disco\n");
      return -1;
   }

   disk.tam_block = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0);
   if(disk.tam_block < 0)
   {
      printf("Falha ao consultar o tamanho do block\n");
      return -1;
   }

   *numBlocks = disk.tam;
   *blockSize = disk.tam_block;

   disk.init = 1;
   disk_wait = 0;
   
   mutex_create(&disk.mrequest);
   mutex_create(&disk.queue_mutex);
   sem_create(&disk.cheio, 0);
   sem_create(&disk.vazio, 0);

   disk.blocos_percorridos = 0;
   disk.tempo_exec = 0;

   disk.pacotes = 0;

   gpstRequestList = createList();

   disk_action.sa_handler = memActionFinished;
   sigemptyset(&disk_action.sa_mask);
   disk_action.sa_flags = 0;

   if(sigaction(SIGUSR1, &disk_action, 0) < 0)
   {
      printf("Erro em sigaction");
      return -1;
   }

   disk.enAlgorithm = CSCAN;

   task_create(&disk.task, diskTaskBody, NULL);
   task_setprio(&disk.task, 0);
   
   return 0;
}

extern int disk_block_read (int block, void *buffer)
{
   addNodeInFront(gpstRequestList, taskExec, block, buffer, DISK_CMD_READ, systemTime);
   
   mutex_lock(&disk.mrequest);
   
   if(disk.pacotes == 0)
   {
      sem_up(&disk.vazio);
      sem_down(&disk.cheio);
   }
   disk.pacotes--;
   
   mutex_unlock(&disk.mrequest);

   return 0;
}

extern int disk_block_write (int block, void *buffer)
{
   addNodeInFront(gpstRequestList, taskExec, block, buffer, DISK_CMD_WRITE, systemTime);

   mutex_lock(&disk.mrequest);

   if(disk.pacotes == 0) {
      sem_up(&disk.vazio);
      sem_down(&disk.cheio);
   }
   disk.pacotes = 0;
   
   mutex_unlock(&disk.mrequest);

   return 0;
}

extern void memActionFinished()
{
   disk.pacotes++;
   sem_up(&disk.cheio);
   disk.tempo_exec += systemTime - disk.tempo_init;

   return;
}

static void diskTaskBody()
{
   while(disk.init == 1)
   {
      sem_down(&disk.vazio);
      disk_scheduler();
   }

   printf("Numero de blocos percorridos %d\nTempo de execução do disco %d ms\n", 
   disk.blocos_percorridos, disk.tempo_exec);
   task_exit(0);
}

///////////////// STATIC FUNCTIONS DESCRIPTIONS /////////////////

ST_RequestNode* fcfs_sched()
{
   return gpstRequestList->firstNode;
}

ST_RequestNode* sstf_sched() {
   int iSize = gpstRequestList->iSize;
   int i = 0;
   ST_RequestNode* pstIter = gpstRequestList->firstNode;

   int head = disk.bloco;
   int shortest = abs(pstIter->block - head);
   ST_RequestNode* pstShortestReq = pstIter;

   int seekDist;
   for(i=0; i < iSize && pstIter != NULL; i++, pstIter=pstIter->next)
   {
      seekDist = abs(pstIter->block - head);
      if(seekDist < shortest)
      {
         shortest = seekDist;
         pstShortestReq = pstIter;
      }
   }

   return pstShortestReq;
}

ST_RequestNode* cscan_sched() {
   int iSize = gpstRequestList->iSize;
   int i     = 0;
   ST_RequestNode *pstIter = gpstRequestList->firstNode;

   int head           = disk.bloco;
   int iShortestFront = pstIter->block - head;
   int iShortest      = pstIter->block;

   ST_RequestNode *pstShortestReq  = pstIter;
   ST_RequestNode *pstShortestReq2 = pstIter;

   int iBlockDist = 0;
   int cIsInFront = 0;
   for (i=0; i < iSize && pstIter != NULL; i++, pstIter=pstIter->next)
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
            iShortest       = iBlockDist;
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

int disk_scheduler() {
   if(isEmpty(gpstRequestList))
   {
      return 0;
   }

   ST_RequestNode *req;
   ST_RequestNode *aux;

   switch(disk.enAlgorithm) {
      case FCFS:
      req = fcfs_sched();
      break;

      case SSTF:
      req = sstf_sched();
      break;

      case CSCAN:
      req = cscan_sched();
      break;

      default :
      req = fcfs_sched();
   }

   int req_block    = req->block;
   void *req_buffer = req->buffer;
   int req_cmd      = req->cTaskAction;
   
   removeNode(gpstRequestList, req);
   
   disk.status = disk_cmd(DISK_CMD_STATUS, 0, 0);
   if(disk.status != DISK_STATUS_IDLE)
   {
      printf("disk_status = %d\n", disk.status);
      return -1;
   }
   
   disk.tempo_init = systemTime;
   disk.blocos_percorridos += abs(req_block - disk.bloco);

   disk.bloco = req_block;
   disk.buffer = req_buffer;

   int result = disk_cmd(req_cmd, disk.bloco, disk.buffer);
   if(result != 0) {
      printf("Falha ao ler/escrever o bloco %d %d %p %d %d\n", disk.bloco, result, disk.buffer, disk.tam, disk.status);
      sem_up(&disk.cheio);
      return -1;
   }

}

static ST_RequestNode *createNode(ST_RequestNode *pstPreviousNode,
                                  ST_RequestNode *pstNextNode,
                                  task_t         *pstTask,
                                  int            block,
                                  int            *buffer,
                                  char           cTaskAction,
                                  unsigned int   uiStartingTick)
{
   ST_RequestNode *pstNewNode = (ST_RequestNode *)malloc(sizeof(ST_RequestNode));
   
   pstNewNode->prev         = pstPreviousNode;
   pstNewNode->next         = pstNextNode;
   pstNewNode->task         = pstTask;
   pstNewNode->block        = block;
   pstNewNode->buffer       = buffer;
   pstNewNode->cTaskAction  = cTaskAction;
   pstNewNode->startingTime = uiStartingTick;

   return pstNewNode;
}

//////////// DOUBLE LINKED LIST FUNCTIONS /////////////

extern ST_RequestList *createList()
{
   // printf("LIST TO BE CREATED\n");
   ST_RequestList *pstNewList = (ST_RequestList *)malloc(sizeof(ST_RequestList));
   pstNewList->firstNode = NULL;
   pstNewList->lastNode  = NULL;
   pstNewList->iSize     = 0;

   // printf("LIST CREATED\n");

   return pstNewList;
}

extern void addNode(ST_RequestList *pstList,
                    ST_RequestNode *pstPreviousNode,
                    task_t         *pstTask,
                    int            block,
                    int            *buffer,
                    char           cTaskAction,
                    unsigned int   uiStartingTick)
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
      pstList->lastNode  = pstNewNode;
   }

   (pstList->iSize)++;

   // printf("NODE ADDED\n");

   return;
}

extern void addNodeInFront(ST_RequestList *pstList,
                           task_t         *pstTask,
                           int            block,
                           int            *buffer,
                           char           cTaskAction,
                           unsigned int   uiStartingTick)
{
   // printf("NODE TO BE ADDED IN FRONT\n");
   if (NULL == pstList)
   {
      return;
   }

   if (NULL == pstList->firstNode)
   {
      ST_RequestNode *pstNewNode = createNode(NULL, NULL, pstTask, block, buffer, cTaskAction, uiStartingTick);

      mutex_lock(&disk.queue_mutex);
      pstList->firstNode = pstNewNode;
      pstList->lastNode  = pstList->firstNode;
      mutex_unlock(&disk.queue_mutex);
   }
   else
   {
      ST_RequestNode *pstNewNode = createNode(pstList->lastNode, NULL, pstTask, block, buffer, cTaskAction, uiStartingTick);
      
      mutex_lock(&disk.queue_mutex);
      pstList->lastNode->next = pstNewNode;
      pstList->lastNode       = pstNewNode;
      mutex_unlock(&disk.queue_mutex);
   }

   (pstList->iSize)++;

   // printf("NODE ADDED IN FRONT\n");

   return;
}

extern void addNodeInBack(ST_RequestList *pstList,
                          task_t         *pstTask,
                          int            block,
                          int            *buffer,
                          char           cTaskAction,
                          unsigned int   uiStartingTick)
{
   // printf("NODE TO BE ADDED IN BACK\n");
   if (NULL == pstList)
   {
      return;
   }

   if (NULL == pstList->firstNode)
   {
      pstList->firstNode = createNode(NULL, NULL, pstTask, block, buffer, cTaskAction, uiStartingTick);
      pstList->lastNode  = pstList->firstNode;
   }
   else
   {
      ST_RequestNode *pstNewNode = createNode(NULL, pstList->firstNode, pstTask, block, buffer, cTaskAction, uiStartingTick);
      pstList->firstNode->prev = pstNewNode;
      pstList->firstNode       = pstNewNode;
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
      pstList->lastNode  = NULL;
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
   // printf("NODE REMOVED\n");

   return;
}

extern char isEmpty(ST_RequestList *pstList)
{
   return (NULL == pstList->firstNode);
}