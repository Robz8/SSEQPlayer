#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sndcommon.h>

// The pitch table from the swiGetPitchTable function, but is bugged if DSi BIOS is set, and table below is different as a result
// Table manually obtained in 5 hours (more or less), through a quickly-made homebrew program :P
u16 pitchTable[0x300] = {
	0x0000, 0x003B, 0x0076, 0x00B2, 0x00ED, 0x0128, 0x0164, 0x019F, 0x01DB, 0x0217, 0x0252, 0x028E, 0x02CA, 0x0305, 0x0341, 0x037D,	// 0x0-0xF
	0x03B9, 0x03F5, 0x0431, 0x046E, 0x04AA, 0x04E6, 0x0522, 0x055F, 0x059B, 0x05D8, 0x0614, 0x0651, 0x068D, 0x06CA, 0x0707, 0x0743,	// 0x10-0x1F
	0x0780, 0x07BD, 0x07FA, 0x0837, 0x0874, 0x08B1, 0x08EF, 0x092C, 0x0969, 0x09A7, 0x09E4, 0x0A21, 0x0A5F, 0x0A9C, 0x0ADA, 0x0B18,	// 0x20-0x2F
	0x0B56, 0x0B93, 0x0BD1, 0x0C0F, 0x0C4D, 0x0C8B, 0x0CC9, 0x0D07, 0x0D45, 0x0D84, 0x0DC2, 0x0E00, 0x0E3F, 0x0E7D, 0x0EBC, 0x0EFA,	// 0x30-0x3F
	0x0F39, 0x0F78, 0x0FB6, 0x0FF5, 0x1034, 0x1073, 0x10B2, 0x10F1, 0x1130, 0x116F, 0x11AE, 0x11EE, 0x122D, 0x126C, 0x12AC, 0x12EB,	// 0x40-0x4F
	0x132B, 0x136B, 0x13AA, 0x13EA, 0x142A, 0x146A, 0x14A9, 0x14E9, 0x1529, 0x1569, 0x15AA, 0x15EA, 0x162A, 0x166A, 0x16AB, 0x16EB,	// 0x50-0x5F
	0x172C, 0x176C, 0x17AD, 0x17ED, 0x182E, 0x186F, 0x18B0, 0x18F0, 0x1931, 0x1972, 0x19B3, 0x19F5, 0x1A36, 0x1A77, 0x1AB8, 0x1AFA,	// 0x60-0x6F
	0x1B3B, 0x1B7D, 0x1BBE, 0x1C00, 0x1C41, 0x1C83, 0x1CC5, 0x1D07, 0x1D48, 0x1D8A, 0x1DCC, 0x1E0E, 0x1E51, 0x1E93, 0x1ED5, 0x1F17,	// 0x70-0x7F
	0x1F5A, 0x1F9C, 0x1FDF, 0x2021, 0x2064, 0x20A6, 0x20E9, 0x212C, 0x216F, 0x21B2, 0x21F5, 0x2238, 0x227B, 0x22BE, 0x2301, 0x2344,	// 0x80-0x8F
	0x2388, 0x23CB, 0x240E, 0x2452, 0x2496, 0x24D9, 0x251D, 0x2561, 0x25A4, 0x25E8, 0x262C, 0x2670, 0x26B4, 0x26F8, 0x273D, 0x2781,	// 0x90-0x9F
	0x27C5, 0x280A, 0x284E, 0x2892, 0x28D7, 0x291C, 0x2960, 0x29A5, 0x29EA, 0x2A2F, 0x2A74, 0x2AB9, 0x2AFE, 0x2B43, 0x2B88, 0x2BCD,	// 0xA0-0xAF
	0x2C13, 0x2C58, 0x2C9D, 0x2CE3, 0x2D28, 0x2D6E, 0x2DB4, 0x2DF9, 0x2E3F, 0x2E85, 0x2ECB, 0x2F11, 0x2F57, 0x2F9D, 0x2FE3, 0x302A,	// 0xB0-0xBF
	0x3070, 0x30B6, 0x30FD, 0x3143, 0x318A, 0x31D0, 0x3217, 0x325E, 0x32A5, 0x32EC, 0x3332, 0x3379, 0x33C1, 0x3408, 0x344F, 0x3496,	// 0xC0-0xCF
	0x34DD, 0x3525, 0x356C, 0x35B4, 0x35FB, 0x3643, 0x368B, 0x36D3, 0x371A, 0x3762, 0x37AA, 0x37F2, 0x383A, 0x3883, 0x38CB, 0x3913,	// 0xD0-0xDF
	0x395C, 0x39A4, 0x39ED, 0x3A35, 0x3A7E, 0x3AC6, 0x3B0F, 0x3B58, 0x3BA1, 0x3BEA, 0x3C33, 0x3C7C, 0x3CC5, 0x3D0E, 0x3D58, 0x3DA1,	// 0xE0-0xEF
	0x3DEA, 0x3E34, 0x3E7D, 0x3EC7, 0x3F11, 0x3F5A, 0x3FA4, 0x3FEE, 0x4038, 0x4082, 0x40CC, 0x4116, 0x4161, 0x41AB, 0x41F5, 0x4240,	// 0xF0-0xFF
	0x428A, 0x42D5, 0x431F, 0x436A, 0x43B5, 0x4400, 0x444B, 0x4495, 0x44E1, 0x452C, 0x4577, 0x45C2, 0x460D, 0x4659, 0x46A4, 0x46F0,	// 0x100-0x10F
	0x473B, 0x4787, 0x47D3, 0x481E, 0x486A, 0x48B6, 0x4902, 0x494E, 0x499A, 0x49E6, 0x4A33, 0x4A7F, 0x4ACB, 0x4B18, 0x4B64, 0x4BB1,	// 0x110-0x11F
	0x4BFE, 0x4C4A, 0x4C97, 0x4CE4, 0x4D31, 0x4D7E, 0x4DCB, 0x4E18, 0x4E66, 0x4EB3, 0x4F00, 0x4F4E, 0x4F9B, 0x4FE9, 0x5036, 0x5084,	// 0x120-0x12F
	0x50D2, 0x5120, 0x516E, 0x51BC, 0x520A, 0x5258, 0x52A6, 0x52F4, 0x5343, 0x5391, 0x53E0, 0x542E, 0x547D, 0x54CC, 0x551A, 0x5569,	// 0x130-0x13F
	0x55B8, 0x5607, 0x5656, 0x56A5, 0x56F4, 0x5744, 0x5793, 0x57E2, 0x5832, 0x5882, 0x58D1, 0x5921, 0x5971, 0x59C1, 0x5A10, 0x5A60,	// 0x140-0x14F
	0x5AB0, 0x5B01, 0x5B51, 0x5BA1, 0x5BF1, 0x5C42, 0x5C92, 0x5CE3, 0x5D34, 0x5D84, 0x5DD5, 0x5E26, 0x5E77, 0x5EC8, 0x5F19, 0x5F6A,	// 0x150-0x15F
	0x5FBB, 0x600D, 0x605E, 0x60B0, 0x6101, 0x6153, 0x61A4, 0x61F6, 0x6248, 0x629A, 0x62EC, 0x633E, 0x6390, 0x63E2, 0x6434, 0x6487,	// 0x160-0x16F
	0x64D9, 0x652C, 0x657E, 0x65D1, 0x6624, 0x6676, 0x66C9, 0x671C, 0x676F, 0x67C2, 0x6815, 0x6869, 0x68BC, 0x690F, 0x6963, 0x69B6,	// 0x170-0x17F
	0x6A0A, 0x6A5E, 0x6AB1, 0x6B05, 0x6B59, 0x6BAD, 0x6C01, 0x6C55, 0x6CAA, 0x6CFE, 0x6D52, 0x6DA7, 0x6DFB, 0x6E50, 0x6EA4, 0x6EF9,	// 0x180-0x18F
	0x6F4E, 0x6FA3, 0x6FF8, 0x704D, 0x70A2, 0x70F7, 0x714D, 0x71A2, 0x71F7, 0x724D, 0x72A2, 0x72F8, 0x734E, 0x73A4, 0x73FA, 0x7450,	// 0x190-0x19F
	0x74A6, 0x74FC, 0x7552, 0x75A8, 0x75FF, 0x7655, 0x76AC, 0x7702, 0x7759, 0x77B0, 0x7807, 0x785E, 0x78B4, 0x790C, 0x7963, 0x79BA,	// 0x1A0-0x1AF
	0x7A11, 0x7A69, 0x7AC0, 0x7B18, 0x7B6F, 0x7BC7, 0x7C1F, 0x7C77, 0x7CCF, 0x7D27, 0x7D7F, 0x7DD7, 0x7E2F, 0x7E88, 0x7EE0, 0x7F38,	// 0x1B0-0x1BF
	0x7F91, 0x7FEA, 0x8042, 0x809B, 0x80F4, 0x814D, 0x81A6, 0x81FF, 0x8259, 0x82B2, 0x830B, 0x8365, 0x83BE, 0x8418, 0x8472, 0x84CB,	// 0x1C0-0x1CF
	0x8525, 0x857F, 0x85D9, 0x8633, 0x868E, 0x86E8, 0x8742, 0x879D, 0x87F7, 0x8852, 0x88AC, 0x8907, 0x8962, 0x89BD, 0x8A18, 0x8A73,	// 0x1D0-0x1DF
	0x8ACE, 0x8B2A, 0x8B85, 0x8BE0, 0x8C3C, 0x8C97, 0x8CF3, 0x8D4F, 0x8DAB, 0x8E07, 0x8E63, 0x8EBF, 0x8F1B, 0x8F77, 0x8FD4, 0x9030,	// 0x1E0-0x1EF
	0x908C, 0x90E9, 0x9146, 0x91A2, 0x91FF, 0x925C, 0x92B9, 0x9316, 0x9373, 0x93D1, 0x942E, 0x948C, 0x94E9, 0x9547, 0x95A4, 0x9602,	// 0x1F0-0x1FF
	0x9660, 0x96BE, 0x971C, 0x977A, 0x97D8, 0x9836, 0x9895, 0x98F3, 0x9952, 0x99B0, 0x9A0F, 0x9A6E, 0x9ACD, 0x9B2C, 0x9B8B, 0x9BEA,	// 0x200-0x20F
	0x9C49, 0x9CA8, 0x9D08, 0x9D67, 0x9DC7, 0x9E26, 0x9E86, 0x9EE6, 0x9F46, 0x9FA6, 0xA006, 0xA066, 0xA0C6, 0xA127, 0xA187, 0xA1E8,	// 0x210-0x21F
	0xA248, 0xA2A9, 0xA30A, 0xA36B, 0xA3CC, 0xA42D, 0xA48E, 0xA4EF, 0xA550, 0xA5B2, 0xA613, 0xA675, 0xA6D6, 0xA738, 0xA79A, 0xA7FC,	// 0x220-0x22F
	0xA85E, 0xA8C0, 0xA922, 0xA984, 0xA9E7, 0xAA49, 0xAAAC, 0xAB0E, 0xAB71, 0xABD4, 0xAC37, 0xAC9A, 0xACFD, 0xAD60, 0xADC3, 0xAE27,	// 0x230-0x23F
	0xAE8A, 0xAEED, 0xAF51, 0xAFB5, 0xB019, 0xB07C, 0xB0E0, 0xB145, 0xB1A9, 0xB20D, 0xB271, 0xB2D6, 0xB33A, 0xB39F, 0xB403, 0xB468,	// 0x240-0x24F
	0xB4CD, 0xB532, 0xB597, 0xB5FC, 0xB662, 0xB6C7, 0xB72C, 0xB792, 0xB7F7, 0xB85D, 0xB8C3, 0xB929, 0xB98F, 0xB9F5, 0xBA5B, 0xBAC1,	// 0x250-0x25F
	0xBB28, 0xBB8E, 0xBBF5, 0xBC5B, 0xBCC2, 0xBD29, 0xBD90, 0xBDF7, 0xBE5E, 0xBEC5, 0xBF2C, 0xBF94, 0xBFFB, 0xC063, 0xC0CA, 0xC132,	// 0x260-0x26F
	0xC19A, 0xC202, 0xC26A, 0xC2D2, 0xC33A, 0xC3A2, 0xC40B, 0xC473, 0xC4DC, 0xC544, 0xC5AD, 0xC616, 0xC67F, 0xC6E8, 0xC751, 0xC7BB,	// 0x270-0x27F
	0xC824, 0xC88D, 0xC8F7, 0xC960, 0xC9CA, 0xCA34, 0xCA9E, 0xCB08, 0xCB72, 0xCBDC, 0xCC47, 0xCCB1, 0xCD1B, 0xCD86, 0xCDF1, 0xCE5B,	// 0x280-0x28F
	0xCEC6, 0xCF31, 0xCF9C, 0xD008, 0xD073, 0xD0DE, 0xD14A, 0xD1B5, 0xD221, 0xD28D, 0xD2F8, 0xD364, 0xD3D0, 0xD43D, 0xD4A9, 0xD515,	// 0x290-0x29F
	0xD582, 0xD5EE, 0xD65B, 0xD6C7, 0xD734, 0xD7A1, 0xD80E, 0xD87B, 0xD8E9, 0xD956, 0xD9C3, 0xDA31, 0xDA9E, 0xDB0C, 0xDB7A, 0xDBE8,	// 0x2A0-0x2AF
	0xDC56, 0xDCC4, 0xDD32, 0xDDA0, 0xDE0F, 0xDE7D, 0xDEEC, 0xDF5B, 0xDFC9, 0xE038, 0xE0A7, 0xE116, 0xE186, 0xE1F5, 0xE264, 0xE2D4,	// 0x2B0-0x2BF
	0xE343, 0xE3B3, 0xE423, 0xE493, 0xE503, 0xE573, 0xE5E3, 0xE654, 0xE6C4, 0xE735, 0xE7A5, 0xE816, 0xE887, 0xE8F8, 0xE969, 0xE9DA,	// 0x2C0-0x2CF
	0xEA4B, 0xEABC, 0xEB2E, 0xEB9F, 0xEC11, 0xEC83, 0xECF5, 0xED66, 0xEDD9, 0xEE4B, 0xEEBD, 0xEF2F, 0xEFA2, 0xF014, 0xF087, 0xF0FA,	// 0x2D0-0x2DF
	0xF16D, 0xF1E0, 0xF253, 0xF2C6, 0xF339, 0xF3AD, 0xF420, 0xF494, 0xF507, 0xF57B, 0xF5EF, 0xF663, 0xF6D7, 0xF74C, 0xF7C0, 0xF834,	// 0x2E0-0x2EF
	0xF8A9, 0xF91E, 0xF992, 0xFA07, 0xFA7C, 0xFAF1, 0xFB66, 0xFBDC, 0xFC51, 0xFCC7, 0xFD3C, 0xFDB2, 0xFE28, 0xFE9E, 0xFF14, 0xFF8A};	// 0x2F0-0x2FF

// This function was obtained through disassembly of Ninty's sound driver
u16 AdjustFreq(u16 basefreq, int pitch)
{
	u64 freq;
	int shift = 0;
	pitch = -pitch;
	while (pitch < 0)
	{
		shift --;
		pitch += 0x300;
	}
	while (pitch >= 0x300)
	{
		shift ++;
		pitch -= 0x300;
	}
	freq = (u64)basefreq * ((u32)pitchTable[pitch] + 0x10000);
	shift -= 16;
	if (shift <= 0)
		freq >>= -shift;
	else if (shift < 32)
	{
		if (freq & ((~0ULL) << (32-shift))) return 0xFFFF;
		freq <<= shift;
	}else
		return 0x10;
	if (freq < 0x10) return 0x10;
	if (freq > 0xFFFF) return 0xFFFF;
	return (u16)freq;
}

static inline u16 ADJUST_FREQ(u16 basefreq, int noteN, int baseN)
{
	return AdjustFreq(basefreq, ((noteN - baseN) * 64));
}

static inline u16 ADJUST_PITCH_BEND(u16 basefreq, int pitchb, int pitchr)
{
	if (!pitchb) return basefreq;
	return AdjustFreq(basefreq, (pitchb*pitchr) >> 1);
}

// info about the sample
typedef struct tagSwavInfo
{
	u8  nWaveType;    // 0 = PCM8, 1 = PCM16, 2 = (IMA-)ADPCM
	u8  bLoop;        // Loop flag = TRUE|FALSE
	u16 nSampleRate;  // Sampling Rate
	u16 nTime;        // (ARM7_CLOCK / nSampleRate) [ARM7_CLOCK: 33.513982MHz / 2 = 1.6756991 E +7]
	u16 nLoopOffset;  // Loop Offset (expressed in words (32-bits))
	u32 nNonLoopLen;  // Non Loop Length (expressed in words (32-bits))
} SWAVINFO;

#define SOUND_FORMAT(a) (((int)(a))<<29)
#define SOUND_LOOP(a) ((a) ? SOUND_REPEAT : SOUND_ONE_SHOT)
#define GETSAMP(a) ((void*)((char*)(a)+sizeof(SWAVINFO)))

SWAVINFO* GetWav(void* war, int id)
{
	return (SWAVINFO*)((int)war + ((int*)((int)war+60))[id]);
}

u32 GetInstr(void* bnk, int id)
{
	return *(u32*)((int)bnk + 60 + 4*id);
}

#define INST_TYPE(a) ((a) & 0xFF)
#define INST_OFF(a) ((a) >> 8)

#define GETINSTDATA(bnk,a) ( (u8*) ( (int)(bnk) + (int)INST_OFF(a) ) )

typedef struct
{
	u8 vol, vel, expr, pan, pitchr;
	s8 pitchb;
	u8 modType, modSpeed, modDepth, modRange;
	u16 modDelay;
} playinfo_t;

typedef struct
{
	u16 wavid;
	u16 warid;
	u8 tnote;
	u8 a,d,s,r;
	u8 pan;
} notedef_t;

int ds_freechn2(int prio)
{
	register int i;
	for(i = 0; i < 16; i ++)
		if (!SCHANNEL_ACTIVE(i) && ADSR_ch[i].state != ADSR_START)
			return i;
	int j = -1, ampl = 1;
	for(i = 0; i < 16; i ++)
		if (ADSR_ch[i].state == ADSR_RELEASE && ADSR_ch[i].ampl < ampl)
			ampl = ADSR_ch[i].ampl, j = i;
	if (j != -1) return j;
	for(i = 0; i < 16; i ++)
		if (ADSR_ch[i].prio < prio)
			return i;
	return -1;
}

int ds_freepsgtonechn(int prio)
{
	register int i;
	for(i = 8; i < 14; i ++)
		if (!SCHANNEL_ACTIVE(i) && ADSR_ch[i].state != ADSR_START)
			return i;
	int j = -1, ampl = 1;
	for(i = 8; i < 14; i ++)
		if (ADSR_ch[i].state == ADSR_RELEASE && ADSR_ch[i].ampl < ampl)
			ampl = ADSR_ch[i].ampl, j = i;
	if (j != -1) return j;
	for(i = 8; i < 14; i ++)
		if (ADSR_ch[i].prio < prio)
			return i;
	return -1;
}

int ds_freepsgnoisechn(int prio)
{
	register int i;
	for(i = 14; i < 16; i ++)
		if (!SCHANNEL_ACTIVE(i) && ADSR_ch[i].state != ADSR_START)
			return i;
	int j = -1, ampl = 1;
	for(i = 14; i < 16; i ++)
		if (ADSR_ch[i].state == ADSR_RELEASE && ADSR_ch[i].ampl < ampl)
			ampl = ADSR_ch[i].ampl, j = i;
	if (j != -1) return j;
	for(i = 14; i < 16; i ++)
		if (ADSR_ch[i].prio < prio)
			return i;
	return -1;
}

typedef struct
{
	int count;
	int pos;
	int ret;
	int prio;
	u16 patch;
	u16 waitmode;
	playinfo_t playinfo;
	int a,d,s,r;
	int loopcount,looppos;
	int track_ended;
	int track_looped;
} trackstat_t;

int ntracks = 0;
u8* seqData = NULL;
void* seqBnk = NULL;
void* seqWar[4] = {NULL, NULL, NULL, NULL};
trackstat_t tracks[16];

int _Note(void* bnk, void** war, int instr, int note, int prio, playinfo_t* playinfo, int duration, int track)
{
#ifdef SNDSYS_DEBUG
	returnMsg msg;
	msg.channel = track;
#endif	
	int isPsg = 0;
	int ch = ds_freechn2(prio);
	if (ch < 0) return -1;

	ADSR_stat_t* chstat = ADSR_ch + ch;
	
	u32 inst = GetInstr(bnk, instr);
	u8* insdata = GETINSTDATA(bnk, inst);
	notedef_t* notedef = NULL;
	SWAVINFO* wavinfo = NULL;
	int fRecord = INST_TYPE(inst);
_ReadRecord:
	if (fRecord == 0) {
#ifdef SNDSYS_DEBUG
		msg.count=2;
		msg.data[0] = 0;
		msg.data[1] = fRecord;
		//fifoSendDatamsg(FIFO_RETURN, sizeof(msg), (u8*)&msg);
#endif
		return -1;
	}
	else if (fRecord == 1) notedef = (notedef_t*) insdata;
	else if (fRecord < 4)
	{
		// PSG
		// fRecord = 2 -> PSG tone, notedef->wavid -> PSG duty
		// fRecord = 3 -> PSG noise
		isPsg = 1;
		notedef = (notedef_t*) insdata;
		if (fRecord == 3)
		{
			ch = ds_freepsgnoisechn(prio);
			if (ch < 0) return -1;
			chstat = ADSR_ch + ch;
			chstat->reg.CR = SOUND_FORMAT_PSG | SCHANNEL_ENABLE;
		}else
		{
#define SOUND_DUTY(n) ((n)<<24)
			ch = ds_freepsgtonechn(prio);
			if (ch < 0) return -1;
			chstat = ADSR_ch + ch;
			chstat->reg.CR = SOUND_FORMAT_PSG | SCHANNEL_ENABLE | SOUND_DUTY(notedef->wavid);
		}
		// TODO: figure out what notedef->tnote means for PSG channels
		chstat->_freq = ADJUST_FREQ(-SOUND_FREQ(440*8), note, 69);
		chstat->reg.TIMER = ADJUST_PITCH_BEND(chstat->_freq, playinfo->pitchb, playinfo->pitchr);
	}
	else if (fRecord == 16)
	{
		if ((insdata[0] <= note) && (note <= insdata[1]))
		{
			int rn = note - insdata[0];
			int offset = 2 + rn*(2+sizeof(notedef_t));
			fRecord = insdata[offset];
			insdata += offset + 2;
			goto _ReadRecord;
		}else return -1;
	}else if (fRecord == 17)
	{
		int reg;
		for(reg = 0; reg < 8; reg ++)
			if (note <= insdata[reg]) break;
		if (reg == 8) return -1;
		
		int offset = 8 + reg*(2+sizeof(notedef_t));
		fRecord = insdata[offset];
		insdata += offset + 2;
		goto _ReadRecord;
	}else 
	{
#ifdef SNDSYS_DEBUG
		msg.count=2;
		msg.data[0] = 0;
		msg.data[1] = fRecord;
		//fifoSendDatamsg(FIFO_RETURN, sizeof(msg), (u8*)&msg);
#endif
		return -1;
	}

	if (!isPsg)
	{
		wavinfo = GetWav(war[notedef->warid], notedef->wavid);
		chstat->reg.CR = SOUND_FORMAT(wavinfo->nWaveType) | SOUND_LOOP(wavinfo->bLoop) | SCHANNEL_ENABLE;
		chstat->reg.SOURCE = (u32)GETSAMP(wavinfo);
		chstat->_freq = ADJUST_FREQ(wavinfo->nTime, note, notedef->tnote);
		chstat->reg.TIMER = ADJUST_PITCH_BEND(chstat->_freq, playinfo->pitchb, playinfo->pitchr);
		chstat->reg.REPEAT_POINT = wavinfo->nLoopOffset;
		chstat->reg.LENGTH = wavinfo->nNonLoopLen;
	}

	trackstat_t* pTrack = tracks + track;
	
	chstat->vol = playinfo->vol;
	chstat->vel = playinfo->vel;
	chstat->expr = playinfo->expr;
	chstat->pan = playinfo->pan;
	chstat->pan2 = notedef->pan;
	chstat->modType = playinfo->modType;
	chstat->modDepth = playinfo->modDepth;
	chstat->modRange = playinfo->modRange;
	chstat->modSpeed = playinfo->modSpeed;
	chstat->modDelay = playinfo->modDelay;
	chstat->modDelayCnt = 0;
	chstat->modCounter = 0;
	chstat->a = (pTrack->a == -1) ? CnvAttk(notedef->a) : pTrack->a;
	chstat->d = (pTrack->d == -1) ? CnvFall(notedef->d) : pTrack->d;
	chstat->s = (pTrack->s == -1) ? CnvSust(notedef->s) : pTrack->s;
	chstat->r = (pTrack->r == -1) ? CnvFall(notedef->r) : pTrack->r;
	chstat->prio = prio;
	chstat->count = duration;
	chstat->track = track;
	chstat->state = ADSR_START;

	return ch;
}

void _NoteStop(int n)
{
	ADSR_ch[n].state = ADSR_RELEASE;
}

#define SEQ_READ8(pos) seqData[(pos)]
#define SEQ_READ16(pos) ((u16)seqData[(pos)] | ((u16)seqData[(pos)+1] << 8))
#define SEQ_READ24(pos) ((u32)seqData[(pos)] | ((u32)seqData[(pos)+1] << 8) | ((u32)seqData[(pos)+2] << 16))

int message_send_flag=0;

static inline void PrepareTrack(int i, int pos)
{
#ifdef SNDSYS_DEBUG
		returnMsg msg;
		msg.count=5;
		msg.data[0] = 5;
		msg.data[1] = 0;
		fifoSendDatamsg(FIFO_RETURN, sizeof(msg), (u8*)&msg);
#endif
	memset(tracks + i, 0, sizeof(trackstat_t));
	tracks[i].pos = pos;
	tracks[i].playinfo.vol = 64;
	tracks[i].playinfo.vel = 64;
	tracks[i].playinfo.expr = 127;
	tracks[i].playinfo.pan = 64;
	tracks[i].playinfo.pitchb = 0;
	tracks[i].playinfo.pitchr = 2;
	tracks[i].playinfo.modType = 0;
	tracks[i].playinfo.modDepth = 0;
	tracks[i].playinfo.modRange = 1;
	tracks[i].playinfo.modSpeed = 16;
	tracks[i].playinfo.modDelay = 10;
	tracks[i].prio = 64;
	tracks[i].track_looped = 0;
	tracks[i].track_ended = 0;
	tracks[i].a = -1; tracks[i].d = -1; tracks[i].s = -1; tracks[i].r = -1;
}

void PlaySeq(data_t* seq, data_t* bnk, data_t* war)
{
	seqBnk = bnk->data;
	seqWar[0] = war[0].data;
	seqWar[1] = war[1].data;
	seqWar[2] = war[2].data;
	seqWar[3] = war[3].data;
	
	//Some tracks alter this, and may cause undesireable effects with playing other tracks later.
	ADSR_mastervolume = 127;
	
	// Load sequence data
	seqData = (u8*)seq->data + ((u32*)seq->data)[6];
	ntracks = 1;
	
	int pos = 0;
	
#ifdef SNDSYS_DEBUG
	returnMsg msg;
	msg.count=3;
	if (*seqData == 0xFE)
		msg.data[0] = 0x03;
	else
		msg.data[0] = 0x04;
	fifoSendDatamsg(FIFO_RETURN, sizeof(msg), (u8*)&msg);
#endif

	if (*seqData == 0xFE)
	// Prepare extra tracks
	for (pos = 3; SEQ_READ8(pos) == 0x93; ntracks ++, pos += 3)
	{
		pos += 2;
		PrepareTrack(ntracks, SEQ_READ24(pos));
	}

	// Prepare first track
	PrepareTrack(0, pos);
	seq_bpm = 120;
	message_send_flag = 0;
}


void StopSeq()
{
	returnMsg msg;
	seq_bpm=0; //stop sound_timer
	//v=0;

	int i=0;
	for(; i < 16; i++){ //stop Note
		ADSR_stat_t* chstat = ADSR_ch + i;
		chstat->state=ADSR_NONE;
		chstat->count = 0;
		chstat->track = -1;
		SCHANNEL_CR(i) = 0;
	}
	msg.count=1;
	msg.data[0] = 6;
	fifoSendDatamsg(FIFO_RETURN, sizeof(msg), (u8*)&msg);
}

volatile int seq_bpm = 0;

void track_tick(int n);

void seq_tick()
{
	int i;
	int looped_twice=0;
	int ended=0;
#ifdef LOG_SEQ
	nocashMessage("Tick!");
#endif

	// Handle note durations
	for (i = 0; i < 16; i ++)
	{
		ADSR_stat_t* chstat = ADSR_ch + i;
		if (chstat->count)
		{
			chstat->count --;
			if (!chstat->count) _NoteStop(i);
		}
	}

	for (i = 0; i < ntracks; i ++)
	{
		track_tick(i);
		if(tracks[i].track_looped >= 2)
			looped_twice++;
		if(tracks[i].track_ended > 0)
			ended++;
	}
	returnMsg msg;
	if(!message_send_flag)
	{
		if(looped_twice == ntracks)
		{
			message_send_flag = 1;
			msg.count = 1;
			msg.data[0] = 7;
			fifoSendDatamsg(FIFO_RETURN, sizeof(msg), (u8*)&msg);
			return;
		}
		if(ended == ntracks)
		{
			message_send_flag = 1;
			msg.count = 1;
			msg.data[0] = 8;
			fifoSendDatamsg(FIFO_RETURN, sizeof(msg), (u8*)&msg);
			return;
		}
		if((looped_twice + ended) >= ntracks)
		{
			message_send_flag = 1;
			msg.count = 1;
			msg.data[0] = 7;
			fifoSendDatamsg(FIFO_RETURN, sizeof(msg), (u8*)&msg);
			return;
		}
	}
}

int read_vl(int* pos)
{
	int v = 0;
	for(;;)
	{
		int data = SEQ_READ8(*pos); (*pos) ++;
		v = (v << 7) | (data & 0x7F);
		if (!(data & 0x80)) break;
	}
	return v;
}

void seq_updatenotes(int track, playinfo_t* info)
{
	int i = 0;
	for (i = 0; i < 16; i ++)
	{
		ADSR_stat_t* chstat = ADSR_ch + i;
		if (chstat->track != track) continue;
		chstat->vol = info->vol;
		chstat->expr = info->expr;
		chstat->pan = info->pan;
	}
}

void seq_updatepitchbend(int track, playinfo_t* info)
{
	int i = 0;
	for (i = 0; i < 16; i ++)
	{
		ADSR_stat_t* chstat = ADSR_ch + i;
		if (chstat->track != track) continue;
		chstat->reg.TIMER = ADJUST_PITCH_BEND(chstat->_freq, info->pitchb, info->pitchr);
	}
}

void seq_updatemodulation(int track, playinfo_t* info, int what)
{
	int i = 0;
	for (i = 0; i < 16; i ++)
	{
		ADSR_stat_t* chstat = ADSR_ch + i;
		if (chstat->track != track) continue;
		if (what & BIT(0)) chstat->modDepth = info->modDepth;
		if (what & BIT(1)) chstat->modSpeed = info->modSpeed;
		if (what & BIT(2)) chstat->modType = info->modType;
		if (what & BIT(3)) chstat->modRange = info->modRange;
		if (what & BIT(4)) chstat->modDelay = info->modDelay;
	}
}

void track_tick(int n)
{
	returnMsg msg;
	trackstat_t* track = tracks + n;

	if (track->count)
	{
		track->count --;
		if (track->count) return;
	}

	while (!track->count)
	{
#ifdef LOG_SEQ
		int oldpos = track->pos;
#endif
		int cmd = SEQ_READ8(track->pos); track->pos ++;
#ifdef LOG_SEQ
		char buf[64];
		siprintf(buf, "%02X-%08X-%X", cmd, (int)(seqData + oldpos), oldpos);
		nocashMessage(buf);
#endif
		msg.count = 0;
		msg.channel = n;
		msg.data[0] = cmd;
		msg.data[1] = SEQ_READ8(track->pos);
		msg.data[2] = SEQ_READ8(track->pos+1);
		msg.data[3] = SEQ_READ8(track->pos+2);
		if (cmd < 0x80)
		{
#ifdef LOG_SEQ
			nocashMessage("NOTE-ON");
#endif
			// NOTE-ON
			u8  vel = SEQ_READ8(track->pos); track->pos ++;
			int len = read_vl(&track->pos);
			if (track->waitmode) track->count = len;

			track->playinfo.vel = vel;
			int handle = _Note(seqBnk, seqWar, track->patch, cmd, track->prio, &track->playinfo, len, n);
			if (handle < 0) continue;
		}else switch(cmd)
		{
			case 0x80: // REST
			{
#ifdef LOG_SEQ
				nocashMessage("REST");
#endif
				track->count = read_vl(&track->pos);
				break;
			}
			case 0x81: // PATCH CHANGE
			{
#ifdef LOG_SEQ
				nocashMessage("PATCH");
#endif
				track->patch = read_vl(&track->pos);
				break;
			}
			case 0x94: // JUMP
			{
#ifdef LOG_SEQ
				nocashMessage("JUMP");
#endif
#ifdef SNDSYS_DEBUG
				msg.count=1;
#endif
				if(track->pos > SEQ_READ24(track->pos))
					track->track_looped++;
				track->pos = SEQ_READ24(track->pos);
				break;
			}
			case 0x95: // CALL
			{
#ifdef LOG_SEQ
				nocashMessage("CALL");
#endif
				int dest = SEQ_READ24(track->pos);
				track->ret = track->pos + 3;
				track->pos = dest;
				break;
			}
			case 0xA0: // RANDOM
			{
				// TODO
				// [statusByte] [min16] [max16]
				track->pos += 5;
				break;
			}
			case 0xA1: // WTF #1
			{
				// TODO
				int t = SEQ_READ8(track->pos); track->pos ++;
				if (t >= 0xB0 && t <= 0xBD) track->pos ++;
				track->pos ++;
				break;
			}
			case 0xA2: // IF
			{
				// TODO
				break;
			}
			case 0xB0:
			case 0xB1:
			case 0xB2:
			case 0xB3:
			case 0xB4:
			case 0xB5:
			case 0xB6:
			case 0xB7:
			case 0xB8:
			case 0xB9:
			case 0xBA:
			case 0xBB:
			case 0xBC:
			case 0xBD:
			{
				// TODO
				track->pos += 3;
				break;
			}
			case 0xFD: // RET
			{
#ifdef LOG_SEQ
				nocashMessage("RET");
#endif
				track->pos = track->ret;
				break;
			}
			case 0xC0: // PAN
			{
#ifdef LOG_SEQ
				nocashMessage("PAN");
#endif
				track->playinfo.pan = SEQ_READ8(track->pos); track->pos ++;
				seq_updatenotes(n, &track->playinfo);
				break;
			}
			case 0xC1: // VOL
			{
#ifdef LOG_SEQ
				nocashMessage("VOL");
#endif
				track->playinfo.vol = SEQ_READ8(track->pos); track->pos ++;
				seq_updatenotes(n, &track->playinfo);
				//msg.count=1;
				break;
			}
			case 0xC2: // MASTER VOL
			{
#ifdef LOG_SEQ
				nocashMessage("MASTER VOL");
#endif
				ADSR_mastervolume = SEQ_READ8(track->pos); track->pos ++;
				//msg.count=1;
				break;
			}
			case 0xC3: // TRANSPOSE
			case 0xC8: // TIE
			case 0xC9: // PORTAMENTO
			case 0xCE: // PORTAMENTO ON/OFF
			case 0xCF: // PORTAMENTO TIME
			case 0xD6: // PRINT VAR
			{
				// TODO
#ifdef LOG_SEQ
				nocashMessage("DUMMY1");
#endif
#ifdef SNDSYS_DEBUG
				msg.count=2;
#endif
				track->pos ++;
				break;
			}
			case 0xC4: // PITCH BEND
			{
#ifdef LOG_SEQ
				nocashMessage("PITCH BEND");
#endif

				track->playinfo.pitchb = (s8)SEQ_READ8(track->pos); track->pos ++;
				seq_updatepitchbend(n, &track->playinfo);
				break;
			}
			case 0xC5: // PITCH BEND RANGE
			{
#ifdef LOG_SEQ
				nocashMessage("PITCH BEND RANGE");
#endif

				track->playinfo.pitchr = SEQ_READ8(track->pos); track->pos ++;
				seq_updatepitchbend(n, &track->playinfo);
				break;
			}
			case 0xC6: // PRIORITY
			{
#ifdef LOG_SEQ
				nocashMessage("PRIORITY");
#endif
				track->prio = SEQ_READ8(track->pos); track->pos ++;
				break;
			}
			case 0xC7: // NOTEWAIT
			{
#ifdef LOG_SEQ
				nocashMessage("NOTEWAIT");
#endif
				track->waitmode = SEQ_READ8(track->pos); track->pos ++;
				break;
			}
			case 0xCA: // MODULATION DEPTH
			{
#ifdef LOG_SEQ
				nocashMessage("MODULATION DEPTH");
#endif
				track->playinfo.modDepth = SEQ_READ8(track->pos); track->pos ++;
				seq_updatemodulation(n, &track->playinfo, BIT(0));
				break;
			}
			case 0xCB: // MODULATION SPEED
			{
#ifdef LOG_SEQ
				nocashMessage("MODULATION SPEED");
#endif
				track->playinfo.modSpeed = SEQ_READ8(track->pos); track->pos ++;
				seq_updatemodulation(n, &track->playinfo, BIT(1));
				break;
			}
			case 0xCC: // MODULATION TYPE
			{
#ifdef LOG_SEQ
				nocashMessage("MODULATION TYPE");
#endif
				track->playinfo.modType = SEQ_READ8(track->pos); track->pos ++;
				seq_updatemodulation(n, &track->playinfo, BIT(2));
				break;
			}
			case 0xCD: // MODULATION RANGE
			{
#ifdef LOG_SEQ
				nocashMessage("MODULATION RANGE");
#endif
				track->playinfo.modRange = SEQ_READ8(track->pos); track->pos ++;
				seq_updatemodulation(n, &track->playinfo, BIT(3));
				break;
			}
			case 0xD0: // ATTACK
			{
#ifdef LOG_SEQ
				nocashMessage("ATTACK");
#endif
				track->a = CnvAttk(SEQ_READ8(track->pos)); track->pos ++;
				break;
			}
			case 0xD1: // DECAY
			{
#ifdef LOG_SEQ
				nocashMessage("DECAY");
#endif
				track->d = CnvFall(SEQ_READ8(track->pos)); track->pos ++;
				break;
			}
			case 0xD2: // SUSTAIN
			{
#ifdef LOG_SEQ
				nocashMessage("SUSTAIN");
#endif
				track->s = CnvSust(SEQ_READ8(track->pos)); track->pos ++;
				break;
			}
			case 0xD3: // RELEASE
			{
#ifdef LOG_SEQ
				nocashMessage("RELEASE");
#endif
				track->r = CnvFall(SEQ_READ8(track->pos)); track->pos ++;
				break;
			}
			case 0xD4: // LOOP START
			{
#ifdef LOG_SEQ
				nocashMessage("LOOP START");
#endif
#ifdef SNDSYS_DEBUG
				msg.count=2;
#endif
				track->loopcount = SEQ_READ8(track->pos); track->pos ++;
				track->looppos = track->pos;
				if(!track->loopcount)
					track->loopcount = -1;
				break;
			}
			case 0xFC: // LOOP END
			{
#ifdef LOG_SEQ
				nocashMessage("LOOP END");
#endif
#ifdef SNDSYS_DEBUG
				msg.count=1;
#endif
				int shouldRepeat = 1;
				if (track->loopcount > 0)
					shouldRepeat = --track->loopcount;
				if (shouldRepeat)
					track->pos = track->looppos;
				if((shouldRepeat == 1) && (track->loopcount == 0))
					track->track_looped++;
				break;
			}
			case 0xD5: // EXPR
			{
#ifdef LOG_SEQ
				nocashMessage("EXPR");
#endif
				track->playinfo.expr = SEQ_READ8(track->pos); track->pos ++;
				seq_updatenotes(n, &track->playinfo);
				break;
			}
			case 0xE0: // MODULATION DELAY
			{
#ifdef LOG_SEQ
				nocashMessage("MODULATION DELAY");
#endif
				track->playinfo.modDelay = SEQ_READ16(track->pos); track->pos += 2;
				seq_updatemodulation(n, &track->playinfo, BIT(4));
				break;
			}
			case 0xE3: // SWEEP PITCH
			{
				// TODO
#ifdef LOG_SEQ
				nocashMessage("DUMMY2");
#endif
#ifdef SNDSYS_DEBUG
				msg.count=3;
#endif
				track->pos += 2;
				break;
			}
			case 0xE1: // TEMPO
			{
#ifdef LOG_SEQ
				nocashMessage("TEMPO");
#endif
				seq_bpm = SEQ_READ16(track->pos); track->pos += 2;
				break;
			}
			case 0xFF: // END
			{
#ifdef LOG_SEQ
				nocashMessage("END");
#endif
#ifdef SNDSYS_DEBUG
				if(!track->track_ended)
				{
					msg.count=1;
					msg.data[0] = cmd;
					fifoSendDatamsg(FIFO_RETURN, sizeof(msg), (u8*)&msg);
				}
#endif
				track->track_ended = 1;
				track->pos --;
				return;
			}
#ifdef SNDSYS_DEBUG
			default:
			{
				msg.count=3;
				msg.data[0] = cmd;
				msg.data[1] = SEQ_READ8(track->pos);
				msg.data[2] = SEQ_READ8(track->pos+1);
				msg.data[3] = SEQ_READ8(track->pos+2);
				break;
			}
#endif
		}
		if(msg.count)
			fifoSendDatamsg(FIFO_RETURN, sizeof(msg), (u8*)&msg);
		
	}
}
