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

extern volatile u8 message_data[];
extern volatile u32 message_pointer;

char* DIRList[5000];
char* CurrentNDS;
char* CurrentSPS;
char* CurrentFileSTR;

u32 Max_RAM = 3800000;

u32 FileCount;
u32 CurrentFile;
u32 i,j;

u32 scrollpos=0;
u32 scrollposcounter=0;
u32 scrollposmax=0;
u32 scrollposdirection=0;
u32 LastFile;
u32 LastSSEQ;

u32 SSEQCount;
u32 CharCount;
u32 CurrentSSEQ;
u32 SSEQNameOffset;
u32 SSEQDataOffset;
char* SSEQList[5000];
u32 SSEQListOffset[5000];

bool SSEQMode = false;
bool PlayMode = false;
bool AutoPlay = false;

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

int argc;
char **argv;
int main(int _argc, char **_argv)
{
	consoleDemoInit();
	InstallSoundSys();

	argc=_argc; argv=_argv;
	if(!fatInitDefault())
	{
		iprintf("Filesystem FAIL");
		for(;;) swiWaitForVBlank();
	}

	CurrentFile = 0;

	ReadDIR();
	ShowDIR();

	CurrentSPS=malloc(768);
	if(readFrontend(CurrentSPS)){
		ReadSPS();
	}else{
		free(CurrentSPS);
		CurrentSPS=NULL;
	}

	for(;;)
	{
		if(CurrentSSEQ != LastSSEQ)
		{
			LastSSEQ = CurrentSSEQ;
			scrollpos = 0;
			scrollposcounter = 10;
		}
		if(CurrentFile != LastFile)
		{
			LastFile = CurrentFile;
			scrollpos = 0;
			scrollposcounter = 10;
		}
		
		if(scrollposcounter==0)
		{
			scrollposcounter=10;
			if(!scrollposdirection)
			{
				//RIGHT
				if(scrollpos<scrollposmax)
				{
					scrollpos++;
				}
				else
				{
					scrollposdirection = 1;
					scrollposcounter = 120;
				}
			}
			else
			{
				//LEFT
				if(scrollpos>0)
				{
					scrollpos--;
				}
				else
				{
					scrollposdirection = 0;
					scrollposcounter = 120;
				}
			}
		}
		else
		{
			scrollposcounter--;
		}
		if(!PlayMode)
		{
			message_pointer = 0;	//Ignore any incoming debug messages.
			if(SSEQMode)
				ShowSSEQ();
			else
				ShowDIR();
		}
		swiWaitForVBlank();
		if(PlayMode)
		{
			j = message_pointer;
			for(i=0;i<j;i+=6)
			{
				if(message_data[i])
				{
#ifdef SNDSYS_DEBUG
					iprintf("cmd = %.2X:%.2X",message_data[i+1],message_data[i+2]);
					if(message_data[i]==3)
						iprintf(":%.2X\n",message_data[i+3]);
					else
						iprintf("\n");
#endif
					switch (message_data[i+1])
					{
#ifdef SNDSYS_DEBUG
						case 0x00:
							iprintf("00 Unrecognized record: %d\n",message_data[i+2]);
							break;
						case 0x04:
							iprintf("%X:",message_data[i+5]);
							iprintf("04 SEQUENCE IS MULTI-TRACK\n");
							break;
						case 0x03:
							iprintf("%X:",message_data[i+5]);
							iprintf("03 SEQUENCE IS SINGLE-TRACK\n");
							break;
						case 0x05:
							iprintf("%X:",message_data[i+5]);
							iprintf("05 CREATED TRACK %d\n",message_data[i+2]);
							break;
#endif
						case 0x06:
							if(PlayMode)
								iprintf("06 SEQUENCE STOPPED\n");
							if(PlayMode && AutoPlay)
							{
								if(CurrentSSEQ < SSEQCount - 1)
			                    {
			                      CurrentSSEQ++;
			                      ReadSSEQ();
			                    }
			                    
							}
							break;
						case 0x07:
							if(PlayMode)
								iprintf("07 Track has looped twice\n");
							if(PlayMode && AutoPlay)
							{
								FadeSeq();
							}
							break;
						case 0x08:
							if(PlayMode)
								iprintf("08 Track has ended\n");
							if(PlayMode && AutoPlay)
							{
								if(CurrentSSEQ < SSEQCount - 1)
			                    {
			                      CurrentSSEQ++;
			                      ReadSSEQ();
			                    }
							}
							break;
							
						case 0xC1: // Track Volume
							iprintf("%X:", message_data[i+5]);
							iprintf("C1     Track Volume: %d\n", message_data[i+2]);
							break;
							
						case 0xC2: // MASTER VOLUME
							iprintf("%X:", message_data[i+5]);
							iprintf("C2    MASTER VOLUME: %d\n", message_data[i+2]);
							break;
							
#ifdef SNDSYS_DEBUG
						case 0x94: // JUMP
							iprintf("%X:",message_data[i+5]);
							iprintf("94     POSITION JUMP:\n");
							break;
						case 0xC3: // TRANSPOSE
							iprintf("C3         TRANSPOSE: %.2X\n",message_data[i+2]);
							break;
						case 0xC8: // TIE
							iprintf("C8               TIE: %.2X\n",message_data[i+2]);
							break;
						case 0xC9: // PORTAMENTO
							iprintf("C9        PORTAMENTO: %.2X\n",message_data[i+2]);
							break;
						case 0xCA: // MODULATION DEPTH
							iprintf("CA  MODULATION DEPTH: %.2X\n",message_data[i+2]);
							break;
						case 0xCB: // MODULATION SPEED
							iprintf("CB  MODULATION SPEED: %.2X\n",message_data[i+2]);
							break;
						case 0xCC: // MODULATION TYPE
							iprintf("CC   MODULATION TYPE: %.2X\n",message_data[i+2]);
							break;
						case 0xCD: // MODULATION RANGE
							iprintf("CD  MODULATION RANGE: %.2X\n",message_data[i+2]);
							break;
						case 0xCE: // PORTAMENTO ON/OFF
							iprintf("CE PORTAMENTO ON/OFF: %.2X\n",message_data[i+2]);
							break;
						case 0xCF: // PORTAMENTO TIME
							iprintf("CF   PORTAMENTO TIME: %.2X\n",message_data[i+2]);
							break;
						case 0xD4: //LOOP START
							iprintf("%X:",message_data[i+5]);
							iprintf("D4        LOOP START: %d\n",message_data[i+2]);
							break;
						case 0xD6: // PRINT VAR
							iprintf("D6         PRINT VAR: %.2X\n",message_data[i+2]);
							break;
							
						case 0xE0: // MODULATION DELAY
							iprintf("E0  MODULATION DELAY: %.2X %.2X\n",message_data[i+2],message_data[i+3]);
							break;
						case 0xE3: // SWEEP PITCH
							iprintf("E3       SWEEP PITCH: %.2X %.2X\n",message_data[i+2],message_data[i+3]);
							break;
						case 0xFC:
							iprintf("%X:",message_data[i+5]);
							iprintf("FC          LOOP END:\n");
							break;
						case 0xFF:
							iprintf("%X:",message_data[i+5]);
							iprintf("FF      END OF TRACK:\n");
							break;
						default:
							iprintf("%.2X  Unrecognized cmd: %.2X %.2X %.2X",message_data[i+1],message_data[i+2],message_data[i+3],message_data[i+4]);
							break;
#endif
					}
				}
			}
			message_pointer -= j;
		}
		scanKeys();
		if (keysDown() & KEY_A)
		{
			if(PlayMode)
			{
				fifoSendDatamsg(FIFO_SNDSYS, sizeof(curr_seq), (u8*) &curr_seq);	//User may have stopped current sequence.
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
					
					CurrentSPS = malloc(23 + strlen(DIRList[CurrentFile])+1);
					strcpy(CurrentSPS, "/data/NDS Music Player/");
					strcat(CurrentSPS, DIRList[CurrentFile]);
					CurrentSPS[23 + strlen(DIRList[CurrentFile])]=0;
						
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
		if (keysDown() & KEY_Y)
		{
			FadeSeq();
		}
		
		if(keysDown() & KEY_SELECT)
		{
			AutoPlay = !AutoPlay;
			if(PlayMode)
			{
				if(AutoPlay)
					iprintf("Auto Play enabled\n");
				else
					iprintf("Auto Play disabled\n");
			}
		}
		
		if(keysDown() & KEY_START)
		{
			PauseSeq();
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
	u32 temp;
	
	if(DIRList[0] == NULL)
	{
		return;
	}

	for(i = CurrentFile; i < CurrentFile + 23; i++)
	{
		temp = 0;
		if (i > FileCount)
		{
			break;
		}
		if(i == CurrentFile)
		{
			temp = scrollpos;
			if(strlen(DIRList[i])<0x1F)
				scrollposmax = 0;
			else
				scrollposmax = strlen(DIRList[i])-0x1F;
			CurrentFileSTR = "*";
		}
		else
		{
			CurrentFileSTR = " ";
		}
		//Prints text on screen
		if(strlen(DIRList[i])<0x1F)
		{
			iprintf("%s%s\n", CurrentFileSTR, DIRList[i]);
		}
		else
		{
			char SSEQStr[0x20];
			memcpy(SSEQStr,&DIRList[i][temp],0x1F);
			SSEQStr[0x1F]=0;
			iprintf("%s%s", CurrentFileSTR, SSEQStr);
		}
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
	//fclose(f);
	
	FILE *g = fopen(CurrentNDS, "rb");

	iprintf("Checking NDS path.\n");
	swiWaitForVBlank();
	if(!g)
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
	//fclose(f);

	//f = fopen(CurrentSPS, "rb");
	
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
	fread(&SSEQDataOffset, 1, 4, f);

	fseek(f, SSEQNameOffset, SEEK_SET);

	//Reads all SSEQ names into the char**
	for (i = 0, j = 0; (i + j) < SSEQCount; i++)
	{
		SSEQNameOffset = ftell(f);
		fseek(f, SSEQDataOffset + ((i+j) * 48), SEEK_SET);
		fread(&SSEQOffset, 1, 4, f);
		fseek(f, SSEQNameOffset, SEEK_SET);
		if(SSEQOffset == 0)
		{
			i--;
			j++;
			CharCount = fgetc(f);
			while(CharCount>0)
			{
				CharCount--;
				fgetc(f);
			}
			continue;
			
		}
		/*fseek(g, SSEQOffset+24, SEEK_SET);
		u32 fileoffset;
		fread(&fileoffset,1,4,g);
		fseek(g, SSEQOffset+fileoffset, SEEK_SET);
		if(fgetc(g) != 0xFE)
		{
			i--;
			j++;
			CharCount = fgetc(f);
			while(CharCount>0)
			{
				CharCount--;
				fgetc(f);
			}
			continue;
		}*/
		
		SSEQListOffset[i] = i+j;
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
	SSEQCount=i;

	CurrentSSEQ = 0;
	SSEQMode = true;
	fclose(f);
	fclose(g);
	ShowSSEQ();
}

//displays the list of SSEQs
void ShowSSEQ()
{
	//Clears screen
	consoleClear();	
	u32 temp;

	//displays the current SSEQ and up to 22 more files after it
	for(i = CurrentSSEQ; i < CurrentSSEQ + 23; i++)
	{
		temp = 0;
		//stops list short if CurrentSSEQ + 23 is over SSEQCount
		if (i >= SSEQCount)
		{
			break;
		}

		//adds a * infront of the selected file
		if(i == CurrentSSEQ)
		{
			temp = scrollpos;
			if(strlen(SSEQList[i])<0x1F)
				scrollposmax = 0;
			else
				scrollposmax = strlen(SSEQList[i]) - 0x1F;
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
			memcpy(SSEQStr,&SSEQList[i][temp],0x1F);
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
	//iprintf("Reading SSEQ data offset.\n");
	//swiWaitForVBlank();
	fread(&SSEQDataOffset, 1, 4, f);

	//Reads SSEQ offset
	fseek(f, SSEQDataOffset + (SSEQListOffset[CurrentSSEQ] * 48), SEEK_SET);
	//iprintf("Reading SSEQ offset.\n");
	//swiWaitForVBlank();
	fread(&SSEQOffset, 1, 4, f);

	//Reads SSEQ size
	//iprintf("Reading SSEQ size.\n");
	//swiWaitForVBlank();
	fread(&SSEQSize, 1, 4, f);

	//Reads BANK offset
	//iprintf("Reading BANK offset.\n");
	//swiWaitForVBlank();
	fread(&BANKOffset, 1, 4, f);

	//Reads BANK size
	//iprintf("Reading BANK size.\n");
	//swiWaitForVBlank();
	fread(&BANKSize, 1, 4, f);

	//Reads WAVEARC1 offset
	//iprintf("Reading WAVEARC1 offset.\n");
	//swiWaitForVBlank();
	fread(&WAVEARC1Offset, 1, 4, f);

	//Reads WAVEARC1 size
	//iprintf("Reading WAVEARC1 size.\n");
	//swiWaitForVBlank();
	fread(&WAVEARC1Size, 1, 4, f);

	//Reads WAVEARC2 offset
	//iprintf("Reading WAVEARC2 offset.\n");
	//swiWaitForVBlank();
	fread(&WAVEARC2Offset, 1, 4, f);

	//Reads WAVEARC2 size
	//iprintf("Reading WAVEARC2 size.\n");
	//swiWaitForVBlank();
	fread(&WAVEARC2Size, 1, 4, f);

	//Reads WAVEARC3 offset
	//iprintf("Reading WAVEARC3 offset.\n");
	//swiWaitForVBlank();
	fread(&WAVEARC3Offset, 1, 4, f);

	//Reads WAVEARC3 size
	//iprintf("Reading WAVEARC3 size.\n");
	//swiWaitForVBlank();
	fread(&WAVEARC3Size, 1, 4, f);

	//Reads WAVEARC4 offset
	//iprintf("Reading WAVEARC4 offset.\n");
	//swiWaitForVBlank();
	fread(&WAVEARC4Offset, 1, 4, f);

	//Reads WAVEARC4 size
	//iprintf("Reading WAVEARC4 size.\n");
	//swiWaitForVBlank();
	fread(&WAVEARC4Size, 1, 4, f);

	//Closes file
	fclose(f);

	//Plays SSEQ
	if(SSEQOffset != 0)
	{
		
		
		if((SSEQSize + BANKSize + WAVEARC1Size + WAVEARC2Size + WAVEARC3Size + WAVEARC4Size) > Max_RAM)
		{
			iprintf("%s is not currently playable.\n", SSEQList[CurrentSSEQ]);
			iprintf("Data size exceeds Ram size\n");
			iprintf("Available Ram = %d\n", Max_RAM);
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
			if(AutoPlay)
				iprintf("Auto Play enabled\n");
			else
				iprintf("Auto Play disabled\n");
			//swiWaitForVBlank();
			PlayMode = true;
			SSEQMode = false;
			swiWaitForVBlank();
			for(i=0;i<message_pointer;i+=6)
			{
				if(message_data[i+1]==6)
					message_data[i]=0;
				if(message_data[i+1]==7)
					message_data[i]=0;
				if(message_data[i+1]==8)
					message_data[i]=0;
			}
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
