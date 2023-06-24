// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.2 -- Julho de 2017

// interface do gerente de disco rígido (block device driver)

#ifndef __DISK_MGR__
#define __DISK_MGR__

#include <stdlib.h>
#include "disk.h"
#include "ppos_data.h"
#include "ppos-core-globals.h"
#include "ppos.h"

// estruturas de dados e rotinas de inicializacao e acesso
// a um dispositivo de entrada/saida orientado a blocos,
// tipicamente um disco rigido.

//////////// DEFINES AND STRUCTURES /////////////

// estrutura que representa um disco no sistema operacional
typedef struct
{
   char init;
   int tam;
   int tam_block;
   int bloco;
   void* buffer;
   int status;
   task_t task;
   // dreq_t req;
   mutex_t mrequest;
   mutex_t queue_mutex;
   semaphore_t vazio;
   semaphore_t cheio;
   short pacotes;
   int sched;

   int tempo_init;
   int blocos_percorridos;
   int tempo_exec;
   // completar com os campos necessarios
} disk_t ;

////////////// EXTERNABLE VARIABLES ////////////////

disk_t disk;

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
                           int            *buffer,
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
 * @brief Returns if the list is empty
 * 
 * @param pstList Pointer to the list
 * @return char   1 if is empty, 0 if not
 */
extern char isEmpty(ST_RequestList *pstList);

/**
 * @brief Function called when the SIGUSR1 signal is fired
 */
extern void memActionFinished();

#endif
