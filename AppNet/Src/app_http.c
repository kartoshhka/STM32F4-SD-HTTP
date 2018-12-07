#include "app_http.h"

/*------- functions prototypes --------*/

FRESULT RewriteDirsAndFiles(struct SD_Obj *objects);
uint16_t SwitchingTags(int Index, char *pcInsert, FRESULT res, uint16_t *tags_data);
FRESULT SDVolume (uint16_t *data);
uint16_t SSI_Handler(int iIndex, char *pcInsert, int iInsertLen);
const char * MSGS_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
FRESULT scan_files (char* path, uint16_t* dirs, uint16_t* files, uint16_t* data);
FRESULT SD_info (char *path, struct SD_Obj *objects);
void SD_writeInfoFile (struct SD_Obj *objects, uint16_t size);
void SD_writeFile (char *buffer, char *fname);

/*-------- GV --------*/

static char const * TAGCHAR[] = {"a","b","c","d","e"};//,"f","g","h","i","j","k"};
static const tCGI MSGS_CGI={"/msg.cgi", MSGS_CGI_Handler};
static tCGI CGI_TAB[1];
//SD Card FATFS
FATFS SDFatFs;  				 /* File system object for SD card logical drive */
//FIL fsd;     						 /* File object */
extern char SDPath[4];   /* SD logical drive path */

/*------- temporary global variables --------*/

//uint16_t http_var1;
char filename[256]; //test writing file to sd
char text[1000];


void SSI_Init(void)
{	
	if(f_mount(&SDFatFs, (TCHAR const*)SDPath, 0) != FR_OK)
  {
    Error_Handler();
  }
	else
	{
		http_set_ssi_handler(SSI_Handler, (char const **)TAGCHAR, NUMOFTAGS);
	}
}

void CGI_Init(void)
{
  CGI_TAB[0] = MSGS_CGI;
	http_set_cgi_handlers(CGI_TAB, 1);
}

FRESULT RewriteDirsAndFiles(struct SD_Obj *objects)
{
	char buff[256] = { 0 };
	return SD_info(buff, objects);
}

uint16_t SwitchingTags(int Index, char *pcInsert, FRESULT res, uint16_t *tags_data)
{
	if (res == FR_OK) 
	{
		sprintf(pcInsert, "%u", tags_data[Index]);
	}
	else
	{
		sprintf(pcInsert, "No SD detected or no files found");
	}
	//if (Index == InsertLen)
		//free(objects);
	return strlen(pcInsert);
}

FRESULT SDVolume (uint16_t *data)
{
	DWORD fre_clust = 0, fre_sect = 0, tot_sect = 0;
	FRESULT result = FR_OK;
	FATFS *fs = &SDFatFs;

	result = f_getfree("/", &fre_clust, &fs);
	if (result == FR_OK)
		data[2] = fs->csize; //Cluster size
	else
		return result;
	tot_sect = (fs->n_fatent - 2) * fs->csize / 2;
	data[3] = tot_sect; //Total SD volume in KB
	fre_sect = fre_clust * fs->csize / 2;
	data[4] = fre_sect; //Free SD volume in KB
	return result;
}

uint16_t SSI_Handler(int iIndex, char *pcInsert, int iInsertLen)
{
	static FRESULT res = FR_OK;
	char buff[256] = { 0 };					 /* Buffer for node in SD */
	uint16_t dirs_counter = 0; /* How many directories are on SD */
	uint16_t files_counter = 0; /* How many files are on SD*/
	static SD_Obj *all_sd_objects;
	static uint16_t size = 0;
	static uint16_t data[NUMOFTAGS] = { 0 };
	if (iIndex == 0)
	{
		free(all_sd_objects);
		res = scan_files(buff, &dirs_counter, &files_counter, data);
		size = dirs_counter+files_counter;
		all_sd_objects = (SD_Obj*)malloc(sizeof(SD_Obj)*size);
		if (res == FR_OK)
			res = SDVolume (data);
		if (RewriteDirsAndFiles(all_sd_objects) == FR_OK)
		{
			SD_writeInfoFile(all_sd_objects, size);
			//res = scan_files(buff, &dirs_counter, &files_counter, data);
			//size = dirs_counter+files_counter;
		}
		return SwitchingTags(iIndex, pcInsert, res, data); 
	}
	else
	{
		return SwitchingTags(iIndex, pcInsert, res, data); 
	}
}

const char * MSGS_CGI_Handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
	//char filename[256]; //writing file to SD (make right file name!!!!! when it have .txt) !!!!!!
	//char text[1000];
  if (iIndex==0)
  {
		for (uint32_t i=0; i < iNumParams; i++)
    {
			  if (strcmp(pcParam[i] , "fileName") == 0)
        {
					strncpy(filename, pcValue[i], strlen(pcValue[i])); 
				}
				if (strcmp(pcParam[i] , "fileText") == 0)
        {
					strncpy(text, pcValue[i], strlen(pcValue[i])); 
					SD_writeFile(text, filename);
				}
		}
	}
	return "/send.html";
}

FRESULT scan_files (char* path, uint16_t* dirs, uint16_t* files, uint16_t *data)        /* Start node to be scanned (***also used as work area***) */
{
	FRESULT res = FR_OK;
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
				res = scan_files(path, dirs, files, data);                    /* Enter the directory */
				if (res != FR_OK) 
					break;
				path[i] = 0;
			} 
			else 
			{ 
				*files += 1;     /* It is a file. */
			}
		}
		f_closedir(&dir);
	}
	data[0] = *dirs;
	data[1] = *files;
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
				sprintf(objects[obj].type, "%s", "d\0");
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
				sprintf(objects[obj].type, "%s", "f\0");
				obj++;
			}
		}
		f_closedir(&dir);
	}
	return res;
}

void SD_writeInfoFile (struct SD_Obj *objects, uint16_t size)
{
	//UINT size = sizeof(objects)/sizeof(SD_Obj);
	FIL MyFile;		/* File object */
	
	/* Writing to file all names of objects on SD */
	if(f_open(&MyFile, "SDInfo.txt", FA_CREATE_ALWAYS | FA_WRITE) != FR_OK)
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

void SD_writeFile (char *buffer, char *fname)
{
	FIL new_file;		/* File object */
	uint32_t byteswritten;
	FRESULT res;
	
	if (fname[0] == '\0') // no file name
		strcpy(fname, "FileWEB");
	
	uint8_t i = strlen(fname);
	if (strcmp(&fname[i-4], ".txt") != 0)
		sprintf(&fname[i], ".txt");
	
	res = f_open(&new_file, fname, FA_CREATE_ALWAYS | FA_WRITE);
	if(res != FR_OK)
	{
		if (res == FR_INVALID_NAME)
		{
			strcpy(fname, "FileWEB.txt"); // invalid file name
			SD_writeFile(buffer, fname);
		}
		else
			Error_Handler();
	}
	else
	{
		res = f_write(&new_file, buffer, sizeof(buffer),(void*)&byteswritten);
		if(byteswritten == 0 || res != FR_OK)
		{
			Error_Handler();
		}
		f_close(&new_file);
	}
}

char * ReadLongFile(void)
{
	FIL MyFile;		/* File object */
	if(f_open(&MyFile, "SDInfo.txt", FA_READ) != FR_OK)
		return 0;
	uint16_t i1 = 0;
  uint32_t ind = 0;
	uint16_t bytesread;
	uint32_t f_size = f_size(&MyFile);
	char *arr = (char*)malloc(sizeof(char)*f_size);
  do
  {
		char str1[256] = {0};
    if(f_size<256)
    {
      i1=f_size;
    }
    else
    {
      i1=256;
    }
    f_size-=i1;
    f_lseek(&MyFile,ind);
		if (f_read(&MyFile, str1, i1, (UINT *)&bytesread) != FR_OK)
			return 0;
		for (int i = ind; i < i1; i++)
		{
			arr[i] = str1[i];
		}
    ind+=i1;
  } while(f_size>0);
	f_close(&MyFile);
	return arr;
}

