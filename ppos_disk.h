// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.2 -- Julho de 2017

// interface do gerente de disco rígido (block device driver)

#ifndef __DISK_MGR__
#define __DISK_MGR__

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <math.h>
#include "disk.h"
#include "ppos.h"
#include "ppos-core-globals.h"

#define DISK_FCFS 0
#define DISK_SSTF 1
#define DISK_CSCAN 2

// estruturas de dados e rotinas de inicializacao e acesso
// a um dispositivo de entrada/saida orientado a blocos,
// tipicamente um disco rigido.

//////////// DEFINES AND STRUCTURES /////////////

/**
 * @brief Enum for all the possible management algorithms
 * 
 * FCFS:  First Come, First Served
 * SSTF:  Shortest Seek Time First
 * CSCAN: Circular SCAN
 */
typedef enum
{
   FCFS = 0,
   SSTF,
   CSCAN
} EN_DiskAlgorithm;

/**
 * @brief Struct that represents a disk in the virtual OS 
 */
typedef struct
{
   char  init;
   int   size;
   int   iBlockSize;
   int   iCurrBlock;
   void* buffer;
   int   status;

   task_t task;
   mutex_t mRequest;
   mutex_t queueMutex;
   semaphore_t emptySem;
   semaphore_t fullSem;
   short pacotes;
   EN_DiskAlgorithm enAlgorithm;

   int startingTime;
   int totalBlockAccess;
   int execTime;
} disk_t;

/**
 * @brief Node structure for a double linked list
 */
typedef struct requestNode
{
   struct requestNode *prev;
   struct requestNode *next;
   
   task_t *task;
   int    block;
   int    *buffer;
   char   cTaskAction;
   unsigned int startingTime;
} ST_RequestNode;

/**
 * @brief Header structure for the double linked list 
 */
typedef struct requestList
{
   ST_RequestNode *firstNode;
   ST_RequestNode *lastNode;
   int iSize;
} ST_RequestList;

////////////// EXTERNABLE VARIABLES ////////////////

////////////// FUNCTIONS DEFINITIONS ///////////////

/**
 * @brief Inicializacao do gerente de disco
 * 
 * @param numBlocks Tamanho do disco, em blocos
 * @param blockSize Tamanho de cada bloco do disco, em bytes
 * @return int -1 em erro ou 0 em sucesso
 */
extern int disk_mgr_init (int *numBlocks, int *blockSize);

// leitura de um bloco, do disco para o buffer
extern int disk_block_read (int block, void *buffer);

// escrita de um bloco, do buffer para o disco
extern int disk_block_write (int block, void *buffer);

/**
 * @brief Create a List object and returns it
 * 
 * @return NULL
 */
extern ST_RequestList *createList();

/**
 * @brief Adds a node anywhere in the list, given a node where it will be put in front
 * 
 * @param pstList         Pointer to the list
 * @param pstPreviousNode The node where the new one will be put in front
 * @param pstTask         The task that requested the action
 * @param block           Block number where the action will be done
 * @param buffer          Buffer where the data will be stored/read
 * @param uiStartingTick  Tick for when the action was requested
 */
extern void addNode(ST_RequestList *pstList,
                    ST_RequestNode *pstPreviousNode,
                    task_t *pstTask,
                    int block,
                    int *buffer,
                    char cTaskAction,
                    unsigned int uiStartingTick);

/**
 * @brief Adds a node in the end of the list
 * 
 * @param pstList        Pointer to the list
 * @param pstTask        The task that requested the action
 * @param block          Block number where the action will be done
 * @param buffer         Buffer where the data will be stored/read
 * @param uiStartingTick Tick for when the action was requested
 */
extern void addNodeInFront(ST_RequestList *pstList,
                           task_t         *pstTask,
                           int            block,
                           void           *buffer,
                           char           cTaskAction,
                           unsigned int   uiStartingTick);

/**
 * @brief Adds a node in the start of the list
 * 
 * @param pstList        Pointer to the list
 * @param pstTask        The task that requested the action
 * @param block          Block number where the action will be done
 * @param buffer         Buffer where the data will be stored/read
 * @param uiStartingTick Tick for when the action was requested
 */
extern void addNodeInBack(ST_RequestList *pstList,
                          task_t         *pstTask,
                          int            block,
                          int            *buffer,
                          char           cTaskAction,
                          unsigned int   uiStartingTick);

/**
 * @brief Removes a node, given it's pointer
 * 
 * @param pstList Pointer to the list
 * @param pstNode Pointer to the node to be removed
 */
extern void removeNode(ST_RequestList *pstList, ST_RequestNode *pstNode);

/**
 * @brief Frees the entire list and ends the disk task
 */
extern char finishDiskTask();

/**
 * @brief Returns if the list is empty
 * 
 * @param pstList Pointer to the list
 * @return char   1 if is empty, 0 if not
 */
extern char isEmpty(ST_RequestList *pstList);

#endif
