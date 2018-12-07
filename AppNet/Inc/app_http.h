#ifndef __APP_HTTP_H
#define __APP_HTTP_H

#include "lwip/apps/httpd.h"
#include "lwip.h"
#include "fatfs.h"
#include "fs.h"
#include "stm32f4xx_hal.h"
#include <string.h>

/*typedef struct SD_Dir {
	char name[256];
	struct SD_Dir *prevDir;
} SD_Dir;

typedef struct SD_File {
	char name[256];
	struct SD_Dir *currentDir;
} SD_File;*/
#define NUMOFTAGS 5

typedef struct SD_Obj {
	char name[256];
	char type[2];
} SD_Obj;

void SSI_Init(void);
void CGI_Init(void);

void Make_HTTP_SDInfo(struct SD_Obj *objects, uint16_t size);
char * ReadLongFile(void);
#endif /* __APP_ETHERNET_H */
