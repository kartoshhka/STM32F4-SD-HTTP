#include "app_http.h"

uint16_t http_var1;

static char const* TAGCHAR[] = {"d","f"};
static char const** TAGS=TAGCHAR;

static const tCGI MSGS_CGI={"/msg.cgi", MSGS_CGI_Handler};
static tCGI CGI_TAB[1];

//SD Card FATFS
FATFS SDFatFs;  				 /* File system object for SD card logical drive */
FIL fsd;     						 /* File object */
extern char SDPath[4];   /* SD logical drive path */


//temporary global variables
struct SD_Dir dirs_info[3];
struct SD_File files_info[7];

void SSI_Init(void)
{
	if(f_mount(&SDFatFs, (TCHAR const*)SDPath, 0) != FR_OK)
  {
    Error_Handler();
  }
  else
  {
		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);
		http_set_ssi_handler(SSI_Handler, (char const **)TAGS, 2);
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
	strcpy(buff, "/");
	uint16_t dirs_counter = 0; /* How many directories are on SD */
	uint16_t files_counter = 0; /* How many files are on SD*/
  res = scan_files(buff, &dirs_counter, &files_counter);
	//struct SD_Dir dirs_info[dirs_counter];
	//struct SD_File files_info[files_counter];
	strcpy(buff1, "/");
	res = SD_info(buff1, dirs_info, files_info);
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
		}
	}
	else
	{
		switch(iIndex)
		{
			case 0:
			{
				sprintf(pcInsert,"No directories founded");
				return strlen(pcInsert);
			}
			case 1:
			{
				sprintf(pcInsert,"No files founded");
				return strlen(pcInsert);
			}
		}
	}
	/*static uint32_t n = 0;
	switch(iIndex)
	{
		case 0:
		{
			n++;
			sprintf(pcInsert,"%u", n);
			return strlen(pcInsert);
		}
		case 1:
		{
			sprintf(pcInsert,"%u",n+5);
			return strlen(pcInsert);
		}
		case 2:
		{
			sprintf(pcInsert,"%u",n+10);
			return strlen(pcInsert);
		}
		case 3:
		{
			sprintf(pcInsert,"%u",n+15);
			return strlen(pcInsert);
		}		
	}*/
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

FRESULT SD_info (char *path, struct SD_Dir *dirs, struct SD_File *files)
{
	FRESULT res;
	DIR dir;
	UINT i = 0;
	static UINT drs = 0;
	static UINT fls = 0;
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
				sprintf(&dirs[drs].name[0], "/%s", fno.fname);
				drs++;
				res = SD_info(path, dirs, files);                    /* Enter the directory */
				if (res != FR_OK) 
					break;
				path[i] = 0;
			} 
			else 
			{ 
				/* It is a file. */
				sprintf(&files[fls].name[0], "/%s", fno.fname);
				//sprintf(&files[fls].currentDir->name[0], "/%s", &dirs[drs].name[0]);
				fls++;
				//printf("%s/%s\n", path, fno.fname);
			}
		}
		f_closedir(&dir);
	}
	return res;
}
