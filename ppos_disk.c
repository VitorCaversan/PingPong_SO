#include "ppos_disk.h"
#include "ppos-core-globals.h"

static ST_RequestList *pstRequestList;

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

//////////////// EXTERNABLE FUNCTIONS DESCRIPTIONS ///////////////

extern void createList(ST_RequestList *pstList)
{
   ST_RequestList newList = {0};
   *pstList = newList;

   return;
}

extern void addNode(ST_RequestList *pstList,
                    ST_RequestNode *pstPreviousNode,
                    task_t         *pstTask,
                    int            block,
                    int            *buffer,
                    char           cTaskAction,
                    unsigned int   uiStartingTick)
{
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

   return;
}

extern void addNodeInFront(ST_RequestList *pstList,
                           task_t         *pstTask,
                           int            block,
                           int            *buffer,
                           char           cTaskAction,
                           unsigned int   uiStartingTick)
{
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
      ST_RequestNode *pstNewNode = createNode(pstList->lastNode, NULL, pstTask, block, buffer, cTaskAction, uiStartingTick);
      pstList->lastNode->next = pstNewNode;
      pstList->lastNode       = pstNewNode;
   }

   return;
}

extern void addNodeInBack(ST_RequestList *pstList,
                          task_t         *pstTask,
                          int            block,
                          int            *buffer,
                          char           cTaskAction,
                          unsigned int   uiStartingTick)
{
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

   return;
}

extern void removeNode(ST_RequestList *pstList, ST_RequestNode *pstNode)
{
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

   return;
}
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
   unsigned char *ucInitBuf = malloc(sizeof(char));
   int iInitSuccess = -1;

   iInitSuccess = disk_cmd(DISK_CMD_INIT, 0, (void *)ucInitBuf);

   *numBlocks = disk_cmd(DISK_CMD_DISKSIZE, 0, ucInitBuf);
   *blockSize = disk_cmd(DISK_CMD_BLOCKSIZE, 0, ucInitBuf);

   createList(pstRequestList);

   return iInitSuccess;
}

extern int disk_block_read (int block, void *buffer)
{
   addNodeInFront(pstRequestList, taskExec, block, buffer, DISK_CMD_READ, systemTime);

   return 0;
}

extern int disk_block_write (int block, void *buffer)
{
   addNodeInFront(pstRequestList, taskExec, block, buffer, DISK_CMD_WRITE, systemTime);

   return 0;
}

///////////////// STATIC FUNCTIONS DESCRIPTIONS /////////////////

static ST_RequestNode *createNode(ST_RequestNode *pstPreviousNode,
                                  ST_RequestNode *pstNextNode,
                                  task_t         *pstTask,
                                  int            block,
                                  int            *buffer,
                                  char           cTaskAction,
                                  unsigned int   uiStartingTick)
{
   ST_RequestNode *pstNewNode = NULL;
   
   pstNewNode->prev         = pstPreviousNode;
   pstNewNode->next         = pstNextNode;
   pstNewNode->task         = pstTask;
   pstNewNode->block        = block;
   pstNewNode->buffer       = buffer;
   pstNewNode->cTaskAction  = cTaskAction;
   pstNewNode->startingTime = uiStartingTick;

   return pstNewNode;
}