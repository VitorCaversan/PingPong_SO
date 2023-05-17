// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.2 -- Julho de 2017

// interface do gerente de disco rígido (block device driver)

#ifndef __DISK_MGR__
#define __DISK_MGR__

#include "disk.h"

// estruturas de dados e rotinas de inicializacao e acesso
// a um dispositivo de entrada/saida orientado a blocos,
// tipicamente um disco rigido.

// estrutura que representa um disco no sistema operacional
typedef struct
{
  // completar com os campos necessarios
} disk_t ;

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

#endif
