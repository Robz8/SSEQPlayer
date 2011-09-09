#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <sndcommon.h>

static void sndsysMsgHandler(int, void*);
#ifdef SNDSYS_DEBUG
static void returnMsgHandler(int, void*);
#endif
sndsysMsg curr_seq;

void InstallSoundSys()
{
	/* Install FIFO */
	fifoSetDatamsgHandler(FIFO_SNDSYS, sndsysMsgHandler, 0);
#ifdef SNDSYS_DEBUG
	fifoSetDatamsgHandler(FIFO_RETURN, returnMsgHandler, 0);
#endif
}

static void sndsysMsgHandler(int bytes, void* user_data)
{
	sndsysMsg msg;
	fifoGetDatamsg(FIFO_SNDSYS, bytes, (u8*) &msg);
}

#ifdef SNDSYS_DEBUG
static void returnMsgHandler(int bytes, void* user_data)
{
	returnMsg msg;
	fifoGetDatamsg(FIFO_RETURN, bytes, (u8*) &msg);
	if(msg.count)
	{
		iprintf("cmd = %.2X:%.2X",msg.data[0],msg.data[1]);
		msg.count-=2;
		if(msg.count)
			iprintf(":%.2X\n",msg.data[2]);
		else
			iprintf("\n");
		msg.count=0;
	}
}
#endif


void free_pdata(data_t* userdata)
{
	if(userdata->size > 0)
	{
		free(userdata->data);
		userdata->size = 0;
	}
}

void free_seq()
{
	free_pdata(&curr_seq.seq);
	free_pdata(&curr_seq.bnk);
	free_pdata(curr_seq.war + 0);
	free_pdata(curr_seq.war + 1);
	free_pdata(curr_seq.war + 2);
	free_pdata(curr_seq.war + 3);
}

/* The following code must be rethought: */

/*
int PlaySmp(sndreg_t* smp, int a, int d, int s, int r, int vol, int vel, int pan)
{
	sndsysMsg msg;
	msg.msg = SNDSYS_PLAY;
	msg.sndreg = *smp;
	msg.a = (u8) a;
	msg.d = (u8) d;
	msg.s = (u8) s;
	msg.r = (u8) r;
	msg.vol = (u8) vol;
	msg.vel = (u8) vel;
	msg.pan = (s8) pan;
	fifoSendDatamsg(FIFO_SNDSYS, sizeof(msg), (u8*) &msg);
	return (int) fifoGetRetValue(FIFO_SNDSYS);
}

void StopSmp(int handle)
{
	sndsysMsg msg;
	msg.msg = SNDSYS_STOP;
	msg.ch = handle;
	fifoSendDatamsg(FIFO_SNDSYS, sizeof(msg), (u8*) &msg);
}
*/

static bool LoadFile(data_t* pData, const char* fname)
{
	FILE* f = fopen(fname, "rb");
	if (!f) return false;
	fseek(f, 0, SEEK_END);
	pData->size = ftell(f);
	rewind(f);
	pData->data = malloc(pData->size);
	fread(pData->data, 1, pData->size, f);
	fclose(f);
	DC_FlushRange(pData->data, pData->size);
	return true;
}

static bool LoadNDS(data_t* pData, const char* fname, const u32 Offset, const u32 Size)
{
	FILE* f = fopen(fname, "rb");
	if (!f) return false;
	fseek(f, Offset, SEEK_SET);
	pData->size = Size;
	pData->data = malloc(Size);
	fread(pData->data, 1, Size, f);
	fclose(f);
	DC_FlushRange(pData->data, Size);
	return true;
}

void PlaySeq(const char* seqFile, const char* bnkFile, const char* war1, const char* war2, const char* war3, const char* war4)
{
	StopSeq();
	free_seq();
	curr_seq.msg = SNDSYS_PLAYSEQ;

	LoadFile(&curr_seq.seq, seqFile);
	LoadFile(&curr_seq.bnk, bnkFile);
	LoadFile(curr_seq.war + 0, war1);
	LoadFile(curr_seq.war + 1, war2);
	LoadFile(curr_seq.war + 2, war3);
	LoadFile(curr_seq.war + 3, war4);

	fifoSendDatamsg(FIFO_SNDSYS, sizeof(curr_seq), (u8*) &curr_seq);
}

void StopSeq()
{
	sndsysMsg msg;
	msg.msg = SNDSYS_STOPSEQ;
	fifoSendDatamsg(FIFO_SNDSYS, sizeof(msg), (u8*) &msg);
}

void PlaySeqNDS(const char* ndsFile, const u32 SSEQOffset, const u32 SSEQSize, const u32 BANKOffset, const u32 BANKSize, const u32 WAVEARC1Offset, const u32 WAVEARC1Size, const u32 WAVEARC2Offset, const u32 WAVEARC2Size, const u32 WAVEARC3Offset, const u32 WAVEARC3Size, const u32 WAVEARC4Offset, const u32 WAVEARC4Size)
{
	StopSeq();
	free_seq();
	curr_seq.msg = SNDSYS_PLAYSEQ;

	iprintf("Loading SSEQ.\n");
	LoadNDS(&curr_seq.seq, ndsFile, SSEQOffset, SSEQSize);

	iprintf("Loading BANK.\n");
	LoadNDS(&curr_seq.bnk, ndsFile, BANKOffset, BANKSize);

	if(WAVEARC1Offset != 0)
	{
		iprintf("Loading WAVEARC1.\n");
		LoadNDS(curr_seq.war + 0, ndsFile, WAVEARC1Offset, WAVEARC1Size);
	}
	
	if(WAVEARC2Offset != 0)
	{
		iprintf("Loading WAVEARC2.\n");
		LoadNDS(curr_seq.war + 1, ndsFile, WAVEARC2Offset, WAVEARC2Size);
	}

	if(WAVEARC3Offset != 0)
	{
		iprintf("Loading WAVEARC3.\n");
		LoadNDS(curr_seq.war + 2, ndsFile, WAVEARC3Offset, WAVEARC3Size);
	}

	if(WAVEARC4Offset != 0)
	{
		iprintf("Loading WAVEARC4.\n");
		LoadNDS(curr_seq.war + 3, ndsFile, WAVEARC4Offset, WAVEARC4Size);
	}

	fifoSendDatamsg(FIFO_SNDSYS, sizeof(curr_seq), (u8*) &curr_seq);
}
