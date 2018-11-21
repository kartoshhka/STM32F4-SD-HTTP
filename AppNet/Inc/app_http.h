#ifndef __APP_HTTP_H
#define __APP_HTTP_H

#include "lwip/apps/httpd.h"
#include "lwip.h"
#include "fatfs.h"
#include "stm32f4xx_hal.h"
#include <string.h>

typedef struct SD_Dir {
    char name[256];
    struct SD_Dir *prevDir;
} SD_Dir;

typedef struct SD_File {
    char name[256];
    struct SD_Dir *currentDir;
} SD_File;

void SSI_Init(void);
void CGI_Init(void);

uint16_t SSI_Handler(int iIndex, char *pcInsert, int iInsertLen);
const char * MSGS_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);

FRESULT scan_files (char* path, uint16_t* dirs, uint16_t* files);
FRESULT SD_info (char *path, struct SD_Dir *dirs, struct SD_File *files);
#endif /* __APP_ETHERNET_H */
