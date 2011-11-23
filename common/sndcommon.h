#pragma once

#define FIFO_SNDSYS FIFO_USER_01
#define FIFO_RETURN FIFO_USER_02

//#define SNDSYS_DEBUG 1

void InstallSoundSys();

enum
{
	/*SNDSYS_PLAY = 1, SNDSYS_STOP,*/ SNDSYS_PLAYSEQ, SNDSYS_STOPSEQ, SNDSYS_FADESEQ, SNDSYS_PAUSESEQ
};

enum
{
	STATUS_PLAYING, STATUS_STOPPED, STATUS_FADING, STATUS_PAUSED
};

typedef struct
{
	u32 CR, SOURCE;
	u16 TIMER, REPEAT_POINT;
	u32 LENGTH;
} sndreg_t;

typedef struct
{
	void* data;
	int size;
} data_t;

typedef struct
{
	int msg;
	union
	{
		/*
		struct
		{
			sndreg_t sndreg;
			u8 a,d,s,r;
			u8 vol, vel; u8 pan; u8 padding;
		};
		int ch;
		*/
		struct
		{
			data_t seq;
			data_t bnk;
			data_t war[4];
		};
	};
} sndsysMsg;

typedef struct
{
	u8 count;
	u8 data[4];
	u8 channel;
} returnMsg;

#define fifoRetWait(ch) while(!fifoCheckValue32(ch))
#define fifoRetValue(ch) fifoGetValue32(ch)

static inline u32 fifoGetRetValue(int ch)
{
	fifoRetWait(ch);
	return fifoRetValue(ch);
}

#ifdef ARM7

enum { ADSR_NONE = 0, ADSR_START, ADSR_ATTACK, ADSR_DECAY, ADSR_SUSTAIN, ADSR_RELEASE };

#define SCHANNEL_ACTIVE(ch) (SCHANNEL_CR(ch) & SCHANNEL_ENABLE)

#define ADSR_K_AMP2VOL 723
#define ADSR_THRESHOLD (ADSR_K_AMP2VOL*128)

typedef struct
{
	int state;
	int vol,vel,expr,pan,pan2;
	int ampl;
	int a,d,s,r;
	int prio;

	int count,track;

	u16 _freq;

	u8 modType, modSpeed, modDepth, modRange;
	u16 modDelay, modDelayCnt, modCounter;
	
	sndreg_t reg;
} ADSR_stat_t;

extern ADSR_stat_t ADSR_ch[16];

volatile extern int seq_bpm;
volatile extern int seq_status;

volatile extern int ADSR_mastervolume;

void seq_tick();

void PlaySeq(data_t* seq, data_t* bnk, data_t* war);
void StopSeq();

int ds_freechn();
int ds_freepsg();
int ds_freenoise();

int CnvAttk(int attk);
int CnvFall(int fall);
int CnvSust(int sust);
u16 AdjustFreq(u16 basefreq, int pitch);
int GetSoundSine(int arg);

#endif

#ifdef ARM9

/*
int PlaySmp(sndreg_t* smp, int a, int d, int s, int r, int vol, int vel, int pan);
void StopSmp(int handle);
*/
extern sndsysMsg curr_seq;
void PlaySeqNDS(const char* ndsFile, const u32 SSEQOffset, const u32 SSEQSize, const u32 BANKOffset, const u32 BANKSize, const u32 WAVEARC1Offset, const u32 WAVEARC1Size, const u32 WAVEARC2Offset, const u32 WAVEARC2Size, const u32 WAVEARC3Offset, const u32 WAVEARC3Size, const u32 WAVEARC4Offset, const u32 WAVEARC4Size);
void PlaySeq(const char*, const char*, const char*, const char*, const char*, const char*);
void StopSeq();

#endif
