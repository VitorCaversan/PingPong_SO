#include "ppos_disk.h"

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

   return iInitSuccess;
}

extern int disk_block_read (int block, void *buffer)
{

}

extern int disk_block_write (int block, void *buffer)
{

}