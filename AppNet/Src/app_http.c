#include "app_http.h"

uint16_t http_var1;

//static char const * TAGCHAR[] = {"d","f"};
//char* TAGCHAR[11];
//static char const** TAGS=TAGCHAR;

static const tCGI MSGS_CGI={"/msg.cgi", MSGS_CGI_Handler};
static tCGI CGI_TAB[1];

//SD Card FATFS
FATFS SDFatFs;  				 /* File system object for SD card logical drive */
//FIL fsd;     						 /* File object */
extern char SDPath[4];   /* SD logical drive path */


//temporary global variables
//SD_Obj all_sd_objects[11];

void SSI_Init(void)
{
	char buff[256] = { 0 };
	uint16_t dirs_counter = 0; /* How many directories are on SD */
	uint16_t files_counter = 0; /* How many files are on SD*/
	
	if(f_mount(&SDFatFs, (TCHAR const*)SDPath, 0) != FR_OK)
  {
    Error_Handler();
  }
	else
	{
		if (scan_files(buff, &dirs_counter, &files_counter) == FR_OK)
		{
			uint16_t size = dirs_counter + files_counter;
			struct SD_Obj* all_sd_objects = (struct SD_Obj*)malloc(sizeof(struct SD_Obj) * size);
			if (SD_info(buff, all_sd_objects) == FR_OK)
			{
				char ** TAGCHAR = (char**)calloc(size, sizeof(char));
				for (int i = 0; i < size; i++)
				{
					TAGCHAR[i] = all_sd_objects[i].type;
				}
				char ** TAGS = TAGCHAR;
				HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);
				http_set_ssi_handler(SSI_Handler, (char const **)TAGS, size);
				free(TAGCHAR);
			}
			else
				Error_Handler();
		}
		else
			Error_Handler();
	}
}

void CGI_Init(void)
{
  CGI_TAB[0] = MSGS_CGI;
	http_set_cgi_handlers(CGI_TAB, 1);
}

uint16_t SSI_Handler(int iIndex, char *pcInsert, int iInsertLen)
{
	FRESULT res;
	char buff[256] = { 0 };					 /* Buffers for node in SD */
	char buff1[256] = { 0 };
	uint16_t dirs_counter = 0; /* How many directories are on SD */
	uint16_t files_counter = 0; /* How many files are on SD*/
  res = scan_files(buff, &dirs_counter, &files_counter);
	struct SD_Obj* all_sd_objects = (struct SD_Obj*)malloc(sizeof(struct SD_Obj) * (dirs_counter + files_counter));
	res = SD_info(buff1, all_sd_objects);
	uint16_t size = sizeof(all_sd_objects)/sizeof(SD_Obj);
	SD_writeFile(all_sd_objects, size);
	if (res == FR_OK) 
	{
		switch(iIndex)
		{
			case 0:
			{
				sprintf(pcInsert,"%u", dirs_counter);
				return strlen(pcInsert);
			}
			case 1:
			{
				sprintf(pcInsert,"%u", files_counter);
				return strlen(pcInsert);
			}
			default:
			{
				sprintf(pcInsert,"%s", "working...");
				return strlen(pcInsert);
			}
		}
	}
	else
	{
		switch(iIndex)
		{
			case 0:
			{
				sprintf(pcInsert,"No directories found");
				return strlen(pcInsert);
			}
			case 1:
			{
				sprintf(pcInsert,"No files found");
				return strlen(pcInsert);
			}
		}
	}
	free(all_sd_objects);
  return 0;
}

const char * MSGS_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
  if (iIndex==0)
  {
		for (uint32_t i=0; i < iNumParams; i++)
    {
			  if (strcmp(pcParam[i] , "Var1")==0)
        {
					http_var1 = atoi(pcValue[i]); 
				}
		}
	}
	return "/send.html";
}

FRESULT scan_files (char* path, uint16_t* dirs, uint16_t* files)        /* Start node to be scanned (***also used as work area***) */
{
	FRESULT res;
	DIR dir;
	UINT i;
	static FILINFO fno;

	res = f_opendir(&dir, path);                       /* Open the directory */
	if (res == FR_OK) 
	{
		for (;;) 
		{
			res = f_readdir(&dir, &fno);                   /* Read a directory item */
			if (res != FR_OK || fno.fname[0] == 0) 
				break;  /* Break on error or end of dir */
			if (fno.fattrib & AM_DIR) 
			{ 
				*dirs += 1;  /* It is a directory */
				i = strlen(path);
				sprintf(&path[i], "/%s", fno.fname);
				res = scan_files(path, dirs, files);                    /* Enter the directory */
				if (res != FR_OK) 
					break;
				path[i] = 0;
			} 
			else 
			{ 
				*files += 1;     /* It is a file. */
				//printf("%s/%s\n", path, fno.fname);
			}
		}
		f_closedir(&dir);
	}
	return res;
}

FRESULT SD_info (char *path, struct SD_Obj *objects)
{
	FRESULT res;
	DIR dir;
	UINT i = 0;
	static UINT obj = 0;
	static FILINFO fno;

	res = f_opendir(&dir, path);                       /* Open the directory */
	if (res == FR_OK) 
	{
		for (;;) 
		{
			res = f_readdir(&dir, &fno);                   /* Read a directory item */
			if (res != FR_OK || fno.fname[0] == 0) 
				break;  /* Break on error or end of dir */
			if (fno.fattrib & AM_DIR) 
			{ 
				/* It is a directory */
				i = strlen(path);
				sprintf(&path[i], "/%s", fno.fname);
				sprintf(objects[obj].name, "%s", path);
				objects[obj].type = "d";
				obj++;
				res = SD_info(path, objects);                    /* Enter the directory */
				if (res != FR_OK) 
					break;
				path[i] = 0;
			} 
			else 
			{ 
				/* It is a file. */
				
				//sprintf(files[fls].name, "/%s", fno.fname);
				//sprintf(files[fls].currentDir->name, "/%s", &dirs[drs].name);
				i = strlen(path);
				sprintf(objects[obj].name, "%s", path);
				sprintf(&objects[obj].name[i], "/%s", fno.fname);
				objects[obj].type = "f";
				obj++;
				//printf("%s/%s\n", path, fno.fname);
			}
		}
		f_closedir(&dir);
	}
	return res;
}

void SD_writeFile (struct SD_Obj *objects, uint16_t size)
{
	//UINT size = sizeof(objects)/sizeof(SD_Obj);
	FIL MyFile;		/* File object */

	/* Writing to file all names of objects on SD */
	if(f_open(&MyFile, "mywrite.txt", FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
	{
		Error_Handler();
	}
	else
	{
		for (int i = 0; i < size; i++)
		{
			if(f_puts(objects[i].name, &MyFile) == -1)
				Error_Handler();
			if(f_puts("\n", &MyFile) == -1)
				Error_Handler();
			else 
				continue;
		}		
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11, GPIO_PIN_RESET);
		f_close(&MyFile); 
	}
}
