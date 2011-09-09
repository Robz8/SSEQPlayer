#include <nds.h>
#include <stdio.h>
#include <sndcommon.h>
#include <filesystem.h>
#include <fat.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

void ReadDIR();
void ShowDIR();
void ReadSPS();
void ShowSSEQ();
void ReadSSEQ();

char* DIRList[5000];
char* CurrentNDS;
char* CurrentSPS;
char* CurrentFileSTR;

u32 FileCount;
u32 CurrentFile;
u32 i;

u32 SSEQCount;
u32 CharCount;
u32 CurrentSSEQ;
u32 SSEQNameOffset;
u32 SSEQDataOffset;
char* SSEQList[5000];

bool SSEQMode = false;
bool PlayMode = false;

u32 SSEQOffset;
u32 BANKOffset;
u32 WAVEARC1Offset;
u32 WAVEARC2Offset;
u32 WAVEARC3Offset;
u32 WAVEARC4Offset;
u32 SSEQSize;
u32 BANKSize;
u32 WAVEARC1Size;
u32 WAVEARC2Size;
u32 WAVEARC3Size;
u32 WAVEARC4Size;

int main()
{
	consoleDemoInit();
	InstallSoundSys();
	
	if(!fatInitDefault())
	{
		iprintf("Filesystem FAIL");
		for(;;) swiWaitForVBlank();
	}

	CurrentFile = 0;

	ReadDIR();
	ShowDIR();

	for(;;)
	{
		swiWaitForVBlank();

		scanKeys();
		if (keysDown() & KEY_A)
		{
			if(PlayMode)
			{

			}
			else
			{
				if(SSEQMode)
				{
					ReadSSEQ();
				}
				else
				{
					if (CurrentSPS != NULL)
					{
						free(CurrentSPS);
					}
					
					CurrentSPS = malloc(23 + strlen(DIRList[CurrentFile]));
					strcpy(CurrentSPS, "/data/NDS Music Player/");
					strcat(CurrentSPS, DIRList[CurrentFile]);
						
					ReadSPS();
				}
			}
		}

		if (keysDown() & KEY_B)
		{
			if(PlayMode)
			{
				PlayMode = false;
				SSEQMode = true;
				ShowSSEQ();
			}
			else
			{
				if(SSEQMode)
				{
					SSEQMode = false;
					ShowDIR();
				}
				else
				{
					ShowDIR();
				}
			}
		}
		
		if (keysDown() & KEY_X)
		{
			StopSeq();
		}

		if (keysDown() & KEY_UP)
		{
			if(PlayMode)
			{
			}
			else
			{
				if(SSEQMode)
				{
					if(CurrentSSEQ > 0)
					{
						CurrentSSEQ--;
					}
					else
					{
						CurrentSSEQ = SSEQCount - 1;
					}

					while(SSEQList[CurrentSSEQ][0] == '<')
					{
						CurrentSSEQ--;
						if(CurrentSSEQ > SSEQCount)
						{
							CurrentSSEQ = SSEQCount - 1;
						}
					}
					ShowSSEQ();
				}
				else
				{
					if(CurrentFile > 0)
					{
						CurrentFile--;
					}
					else
					{
						CurrentFile = FileCount;
					}
					ShowDIR();
				}
			}
		}

		if (keysDown() & KEY_DOWN)
		{
			if(PlayMode)
			{
			}
			else
			{
				if(SSEQMode)
				{
					if(CurrentSSEQ < SSEQCount - 1)
					{
						CurrentSSEQ++;
					}
					else
					{
						CurrentSSEQ = 0;
					}
					while(SSEQList[CurrentSSEQ][0] == '<')
					{
						CurrentSSEQ++;
						if(CurrentSSEQ > SSEQCount - 1)
						{
							CurrentSSEQ = 0;
						}
					}
					ShowSSEQ();
				}
				else
				{
					if(CurrentFile < FileCount)
					{
						CurrentFile++;
					}
					else
					{
						CurrentFile = 0;
					}
					ShowDIR();
				}
			}
		}

		if (keysDown() & KEY_LEFT)
		{
			if(PlayMode)
			{
			}
			else
			{
				if(SSEQMode)
				{
					if(CurrentSSEQ >= 23)
					{
						CurrentSSEQ -= 23;
					}
					else
					{
						CurrentSSEQ = 0;
						while(SSEQList[CurrentSSEQ][0] == '<')
						{
							CurrentSSEQ++;
							if(CurrentSSEQ < 0)
							{
								CurrentSSEQ = 0;
							}
						}
					}
					while(SSEQList[CurrentSSEQ][0] == '<')
					{
						CurrentSSEQ--;
						if(CurrentSSEQ < 0)
						{
							CurrentSSEQ = SSEQCount - 1;
						}
					}
					ShowSSEQ();
				}
				else
				{
					if(CurrentFile >= 23)
					{
						CurrentFile -= 23;
					}
					else
					{
						CurrentFile = 0;
					}
					ShowDIR();
				}
			}
		}

		if (keysDown() & KEY_RIGHT)
		{
			if(PlayMode)
			{
			}
			else
			{
				if(SSEQMode)
				{
					if(CurrentSSEQ < (SSEQCount - 24))
					{
						CurrentSSEQ += 23;
					}
					else
					{
						CurrentSSEQ = SSEQCount - 1;
						while(SSEQList[CurrentSSEQ][0] == '<')
						{
							CurrentSSEQ--;
							if(CurrentSSEQ > SSEQCount - 1)
							{
								CurrentSSEQ = SSEQCount - 1;;
							}
						}
					}
					while(SSEQList[CurrentSSEQ][0] == '<')
					{
						CurrentSSEQ++;
						if(CurrentSSEQ > SSEQCount - 1)
						{
							CurrentSSEQ = 0;
						}
					}
					ShowSSEQ();
				}
				else
				{
					if(CurrentFile < (FileCount - 24))
					{
						CurrentFile += 23;
					}
					else
					{
						CurrentFile = FileCount;
					}
					ShowDIR();
				}
			}
		}

                if (keysDown() & KEY_L)
                {
                  if (PlayMode)
                  {
                    if(CurrentSSEQ > 0)
                    {
                      CurrentSSEQ--;
                    }
                    else
                    {
                      CurrentSSEQ = SSEQCount - 1;
                    }

                    while(SSEQList[CurrentSSEQ][0] == '<')
                    {
                      CurrentSSEQ--;
                      if(CurrentSSEQ > SSEQCount)
                      {
                        CurrentSSEQ = SSEQCount - 1;
                      }
                    }
                    ReadSSEQ();
                  }
                }

                if (keysDown() & KEY_R)
                {
                  if (PlayMode)
                  {
                    if(CurrentSSEQ < SSEQCount - 1)
                    {
                      CurrentSSEQ++;
                    }
                    else
                    {
                      CurrentSSEQ = 0;
                    }
                    while(SSEQList[CurrentSSEQ][0] == '<')
                    {
                      CurrentSSEQ++;
                      if(CurrentSSEQ > SSEQCount - 1)
                      {
                        CurrentSSEQ = 0;
                      }
                    }
                    ReadSSEQ();
                  }
                }
	}
}

int IsValidSPS(char *SPS)
{
	FILE* f = fopen(SPS, "rb");

	fseek(f, 0x0C, SEEK_SET);
	CharCount = fgetc(f);

	if (CurrentNDS != NULL)
	{
		free(CurrentNDS);
	}

	CurrentNDS = malloc(CharCount + 1);
	fread(CurrentNDS, 1, CharCount, f);
	CurrentNDS[CharCount] = 0;
	fclose(f);
	
	f = fopen(CurrentNDS, "rb");
	if(!f)
	{
		iprintf(":( ");
		return 0;
	}
	iprintf(":) ");
	fclose(f);
	return 1;
}

void ReadDIR()
{
	FileCount = 0;
	u32 InvalidSPS = 0;
	DIR *pdir;
	struct dirent *pent;
	struct stat statbuf;
	char *SPSPath=NULL;

	pdir = opendir("/data/NDS Music Player/");
	if (pdir)
	{
		while ((pent = readdir(pdir)) != NULL)
		{
			stat(pent -> d_name, &statbuf);
			//if((pent -> d_name[strlen(pent -> d_name) - 4] == '.') && (pent -> d_name[strlen(pent -> d_name) - 3] == 's') && (pent -> d_name[strlen(pent -> d_name) - 2] == 'p') && (pent -> d_name[strlen(pent -> d_name) - 1] == 's'))
			//{
			if((pent-> d_name[0] == '.') && (strlen(pent -> d_name) == 1))
				continue;
			if((pent-> d_name[0] == '.') && (pent-> d_name[1] == '.') && (strlen(pent -> d_name) == 2))
				continue;
			if(SPSPath != NULL)
				free(SPSPath);
			SPSPath=malloc(23+strlen(pent -> d_name) + 1);
			strcpy(SPSPath, "/data/NDS Music Player/");
			strcat(SPSPath, pent->d_name);
			if(IsValidSPS(SPSPath))
			{
				if (DIRList[FileCount] != NULL)
				{
					free(DIRList[FileCount]);
				}
				DIRList[FileCount] = malloc(strlen(pent -> d_name) + 1);
				strcpy(DIRList[FileCount], pent -> d_name);
				FileCount++;
			}
			else
			{
				InvalidSPS = 1;
			}
			//}
		}
		closedir(pdir);
		FileCount--;
		if(InvalidSPS)
		{
			iprintf("At least one SPS no longer has\nIts matching rom FILE\nPlease Run SPS Maker Again.\n\nPress B to continue");
			swiWaitForVBlank();
			while(!(keysDown() & KEY_B))
			{
				scanKeys();
				swiWaitForVBlank();
			}
		}
	}
	else
	{
		iprintf("/data/NDS Music Player/ was not found.\nPlease run SPS Maker.");
		while(1)
		{
			swiWaitForVBlank();
		}
	}
}

void ShowDIR()
{
	consoleClear();
	
	if(DIRList[0] == NULL)
	{
		return;
	}

	for(i = CurrentFile; i < CurrentFile + 23; i++)
	{
		if (i > FileCount)
		{
			break;
		}
		if(i == CurrentFile)
		{
			CurrentFileSTR = "*";
		}
		else
		{
			CurrentFileSTR = " ";
		}
		iprintf("%s%s\n", CurrentFileSTR, DIRList[i]);
	}
}

void ReadSPS()
{
	consoleClear();

	FILE* f = fopen(CurrentSPS, "rb");
	if(!f)
	{
		iprintf("%s failed to open\n",CurrentSPS);
		return;
	}

	fseek(f, 0x0C, SEEK_SET);
	iprintf("Reading NDS path length.\n");
	swiWaitForVBlank();
	CharCount = fgetc(f);

	if (CurrentNDS != NULL)
	{
		free(CurrentNDS);
	}

	CurrentNDS = malloc(CharCount + 1);
	iprintf("Reading NDS path.\n");
	swiWaitForVBlank();
	fread(CurrentNDS, 1, CharCount, f);
	CurrentNDS[CharCount] = 0;
	fclose(f);
	
	f = fopen(CurrentNDS, "rb");

	iprintf("Checking NDS path.\n");
	swiWaitForVBlank();
	if(!f)
	{
		iprintf("%s\n", CurrentNDS);
		iprintf("ROM not in specified location.\nPlease run SPS Maker.");
		swiWaitForVBlank();
		while(!(keysDown() & KEY_B))
		{
			scanKeys();
			swiWaitForVBlank();
		}
		return;
	}
	else
	{
		iprintf("NDS path valid.\n");
		swiWaitForVBlank();
	}
	fclose(f);

	f = fopen(CurrentSPS, "rb");
	
	//Reads SSEQCount
	fseek(f, 0, SEEK_SET);
	iprintf("Reading SSEQ count.\n");
	swiWaitForVBlank();
	fread(&SSEQCount, 1, 4, f);

	//Reads SSEQNameOffset
	fseek(f, 0x04, SEEK_SET);
	iprintf("Reading SSEQ name offset.\n");
	swiWaitForVBlank();
	fread(&SSEQNameOffset, 1, 4, f);

	fseek(f, SSEQNameOffset, SEEK_SET);

	//Reads all SSEQ names into the char**
	for (i = 0; i < SSEQCount; i++)
	{
		if (SSEQList[i] != NULL)
		{
			free(SSEQList[i]);
		}

		CharCount = fgetc(f);
		SSEQList[i] = malloc(CharCount + 1);
		/*iprintf("Reading SSEQ_%d name.\n", i);
		swiWaitForVBlank();*/
		fread(SSEQList[i], 1, CharCount, f);
		SSEQList[i][CharCount] = 0;
	}

	CurrentSSEQ = 0;
	SSEQMode = true;
	fclose(f);
	ShowSSEQ();
}

//displays the list of SSEQs
void ShowSSEQ()
{
	//Clears screen
	consoleClear();	

	//displays the current SSEQ and up to 22 more files after it
	for(i = CurrentSSEQ; i < CurrentSSEQ + 23; i++)
	{
		//stops list short if CurrentSSEQ + 23 is over SSEQCount
		if (i >= SSEQCount)
		{
			break;
		}

		//adds a * infront of the selected file
		if(i == CurrentSSEQ)
		{
			CurrentFileSTR = "*";
		}
		else
		{
			CurrentFileSTR = " ";
		}

		//Prints text on screen
		if(strlen(SSEQList[i])<0x1F)
		{
			iprintf("%s%s\n", CurrentFileSTR, SSEQList[i]);
		}
		else
		{
			char SSEQStr[0x20];
			memcpy(SSEQStr,SSEQList[i],0x1F);
			SSEQStr[0x1F]=0;
			iprintf("%s%s", CurrentFileSTR, SSEQStr);
		}
	}
}

void ReadSSEQ()
{
	//Clears screen
	consoleClear();

	//Opens file
	FILE* f = fopen(CurrentSPS, "rb");
	if(!f)
	{
		iprintf("%s failed to open\n",CurrentSPS);
		return;
	}

	//Reads SSEQDataOffset
	fseek(f, 0x08, SEEK_SET);
	iprintf("Reading SSEQ data offset.\n");
	swiWaitForVBlank();
	fread(&SSEQDataOffset, 1, 4, f);

	//Reads SSEQ offset
	fseek(f, SSEQDataOffset + (CurrentSSEQ * 48), SEEK_SET);
	iprintf("Reading SSEQ offset.\n");
	swiWaitForVBlank();
	fread(&SSEQOffset, 1, 4, f);

	//Reads SSEQ size
	iprintf("Reading SSEQ size.\n");
	swiWaitForVBlank();
	fread(&SSEQSize, 1, 4, f);

	//Reads BANK offset
	iprintf("Reading BANK offset.\n");
	swiWaitForVBlank();
	fread(&BANKOffset, 1, 4, f);

	//Reads BANK size
	iprintf("Reading BANK size.\n");
	swiWaitForVBlank();
	fread(&BANKSize, 1, 4, f);

	//Reads WAVEARC1 offset
	iprintf("Reading WAVEARC1 offset.\n");
	swiWaitForVBlank();
	fread(&WAVEARC1Offset, 1, 4, f);

	//Reads WAVEARC1 size
	iprintf("Reading WAVEARC1 size.\n");
	swiWaitForVBlank();
	fread(&WAVEARC1Size, 1, 4, f);

	//Reads WAVEARC2 offset
	iprintf("Reading WAVEARC2 offset.\n");
	swiWaitForVBlank();
	fread(&WAVEARC2Offset, 1, 4, f);

	//Reads WAVEARC2 size
	iprintf("Reading WAVEARC2 size.\n");
	swiWaitForVBlank();
	fread(&WAVEARC2Size, 1, 4, f);

	//Reads WAVEARC3 offset
	iprintf("Reading WAVEARC3 offset.\n");
	swiWaitForVBlank();
	fread(&WAVEARC3Offset, 1, 4, f);

	//Reads WAVEARC3 size
	iprintf("Reading WAVEARC3 size.\n");
	swiWaitForVBlank();
	fread(&WAVEARC3Size, 1, 4, f);

	//Reads WAVEARC4 offset
	iprintf("Reading WAVEARC4 offset.\n");
	swiWaitForVBlank();
	fread(&WAVEARC4Offset, 1, 4, f);

	//Reads WAVEARC4 size
	iprintf("Reading WAVEARC4 size.\n");
	swiWaitForVBlank();
	fread(&WAVEARC4Size, 1, 4, f);

	//Closes file
	fclose(f);

	//Plays SSEQ
	if(SSEQOffset != 0)
	{
		
		
		if((SSEQSize + BANKSize + WAVEARC1Size + WAVEARC2Size + WAVEARC3Size + WAVEARC4Size) > 3800000)
		{
			iprintf("%s is not currently playable.\n", SSEQList[CurrentSSEQ]);
			iprintf("Data size exceeds Ram size\n");
			iprintf("Available Ram = 3800000\n");
			iprintf("Data size = %d\n",(SSEQSize + BANKSize + WAVEARC1Size + WAVEARC2Size + WAVEARC3Size + WAVEARC4Size));
			while(!(keysDown() & KEY_B))
			{
				scanKeys();
				swiWaitForVBlank();
			}
			while((keysDown() & KEY_B))
			{
				scanKeys();
				swiWaitForVBlank();
			}
			PlayMode = false;
			SSEQMode = true;
			ShowSSEQ();
		}
		else
		{
			consoleClear();
			PlaySeqNDS(CurrentNDS, SSEQOffset, SSEQSize, BANKOffset, BANKSize, WAVEARC1Offset, WAVEARC1Size, WAVEARC2Offset, WAVEARC2Size, WAVEARC3Offset, WAVEARC3Size, WAVEARC4Offset, WAVEARC4Size);
			
			iprintf("Playing %s.\n", SSEQList[CurrentSSEQ]);
			swiWaitForVBlank();
			PlayMode = true;
			SSEQMode = false;
		}
	}
	else
	{
		iprintf("%s seems to be a dummy file.\n", SSEQList[CurrentSSEQ]);
		while(!(keysDown() & KEY_B))
		{
			scanKeys();
			swiWaitForVBlank();
		}
		ShowSSEQ();
	}
}
