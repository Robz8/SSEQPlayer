#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sndcommon.h>

#define ADJUST_FREQ(baseFreq,noteN,baseN) (((int)(baseFreq)*(int)freqTable[noteN])/(int)freqTable[baseN])

static const u16 freqTable[128] = {
	8, 9, 9, 10, 11, 11, 12, 13, 14, 15, 15,
	16, 17, 18, 19, 21, 22, 23, 24, 26, 28, 29, 31,
	33, 35, 37, 39, 41, 44, 46, 49, 52, 55, 58, 62,
	65, 69, 73, 78, 83, 87, 92, 98, 104, 110, 117, 123,
	131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247,
	262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494,
	523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988,
	1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976,
	2093, 2217, 2349, 2489, 2637, 2794, 2969, 3136, 3322, 3520, 3729, 3951,
	4185, 4435, 4599, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902,
	8372, 8870, 9397, 9956, 10548, 11175, 11840, 12544
};

static const u16 pitchBendTable[256] = {
	0, 29, 59, 88, 118, 148, 177, 207,
	237, 266, 296, 326, 355, 385, 415, 445,
	474, 504, 534, 564, 594, 624, 653, 683,
	713, 743, 773, 803, 833, 863, 893, 923,
	953, 983, 1013, 1043, 1073, 1103, 1133, 1163,
	1193, 1223, 1253, 1284, 1314, 1344, 1374, 1404,
	1435, 1465, 1495, 1525, 1556, 1586, 1616, 1646,
	1677, 1707, 1737, 1768, 1798, 1829, 1859, 1889,
	1920, 1950, 1981, 2011, 2042, 2072, 2103, 2133,
	2164, 2194, 2225, 2256, 2286, 2317, 2347, 2378,
	2409, 2439, 2470, 2501, 2531, 2562, 2593, 2624,
	2654, 2685, 2716, 2747, 2778, 2808, 2839, 2870,
	2901, 2932, 2963, 2994, 3025, 3056, 3087, 3118,
	3149, 3180, 3211, 3242, 3273, 3304, 3335, 3366,
	3397, 3428, 3459, 3490, 3521, 3553, 3584, 3615,
	3646, 3677, 3709, 3740, 3771, 3803, 3834, 3865,
	61857, 61885, 61913, 61941, 61969, 61997, 62025, 62053,
	62081, 62109, 62137, 62165, 62193, 62221, 62249, 62277,
	62305, 62334, 62362, 62390, 62418, 62446, 62474, 62503,
	62531, 62559, 62587, 62616, 62644, 62672, 62700, 62729,
	62757, 62785, 62814, 62842, 62870, 62899, 62927, 62956,
	62984, 63012, 63041, 63069, 63098, 63126, 63155, 63183,
	63212, 63240, 63269, 63297, 63326, 63355, 63383, 63412,
	63440, 63469, 63498, 63526, 63555, 63584, 63612, 63641,
	63670, 63699, 63727, 63756, 63785, 63814, 63842, 63871,
	63900, 63929, 63958, 63987, 64016, 64044, 64073, 64102,
	64131, 64160, 64189, 64218, 64247, 64276, 64305, 64334,
	64363, 64392, 64421, 64450, 64479, 64509, 64538, 64567,
	64596, 64625, 64654, 64683, 64713, 64742, 64771, 64800,
	64830, 64859, 64888, 64917, 64947, 64976, 65005, 65035,
	65064, 65093, 65123, 65152, 65182, 65211, 65240, 65270,
	65299, 65329, 65358, 65388, 65417, 65447, 65476, 65506
};

static inline int ADJUST_FREQ_2(int baseFreq, int noteN, int baseN, int pitchb, int pitchr)
{
	if (pitchb == 0) return ADJUST_FREQ(baseFreq, noteN, baseN);
	else
	{
		s64 freq = ADJUST_FREQ(baseFreq, noteN, baseN);
		int i;
		for (i = 0; i < pitchr; i ++)
		{
			freq *= (s64)pitchBendTable[(u8)pitchb] | (pitchb >= 0 ? 0x10000 : 0);
			freq >>= 16;
		}
		return (int)freq;
	}
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
} trackstat_t;

int ntracks = 0;
u8* seqData = NULL;
void* seqBnk = NULL;
void* seqWar[4] = {NULL, NULL, NULL, NULL};
trackstat_t tracks[16];

int _Note(void* bnk, void** war, int instr, int note, int prio, playinfo_t* playinfo, int duration, int track)
{
	int isPsg = 0;
	int ch = ds_freechn2(prio);
	if (ch < 0) return -1;

	ADSR_stat_t* chstat = ADSR_ch + ch;
	
	u32 inst = GetInstr(bnk, instr);
	u8* insdata = GETINSTDATA(bnk, inst);
	notedef_t* notedef = NULL;
	SWAVINFO* wavinfo = NULL;
	int fRecord = INST_TYPE(inst);
	if (fRecord == 0) return -1;
	else if (fRecord == 1) notedef = (notedef_t*) insdata;
	else if (fRecord < 4)
	{
		// PSG
		// fRecord = 2 -> PSG tone, notedef->wavid -> PSG duty
		// fRecord = 3 -> PSG noise
		isPsg = 1;
		notedef = (notedef_t*) insdata;
		int basefreq = freqTable[notedef->tnote-1]<<3; // It needs a -1 for some reason (??)
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
		chstat->reg.TIMER = SOUND_FREQ(ADJUST_FREQ_2(basefreq, note, notedef->tnote, playinfo->pitchb, playinfo->pitchr));
		chstat->_freq = basefreq;
		chstat->_noteR = note;
		chstat->_noteT = notedef->tnote;
	}
	else if (fRecord == 16)
	{
		if ((insdata[0] <= note) && (note <= insdata[1]))
		{
			int rn = note - insdata[0];
			notedef = (notedef_t*) (insdata + 2 + 2 + rn*(2+sizeof(notedef_t)));
		}else return -1;
	}else if (fRecord == 17)
	{
		int reg;
		for(reg = 0; reg < 8; reg ++)
			if (note <= insdata[reg]) break;
		if (reg == 8) return -1;
		notedef = (notedef_t*) (insdata + 8 + 2 + reg*(2+sizeof(notedef_t)));
	}else return -1;

	if (!isPsg)
	{
		wavinfo = GetWav(war[notedef->warid], notedef->wavid);
		chstat->reg.CR = SOUND_FORMAT(wavinfo->nWaveType) | SOUND_LOOP(wavinfo->bLoop) | SCHANNEL_ENABLE;
		chstat->reg.SOURCE = (u32)GETSAMP(wavinfo);
		chstat->reg.TIMER = SOUND_FREQ(ADJUST_FREQ_2((int)wavinfo->nSampleRate, note, notedef->tnote, playinfo->pitchb, playinfo->pitchr));
		chstat->reg.REPEAT_POINT = wavinfo->nLoopOffset;
		chstat->reg.LENGTH = wavinfo->nNonLoopLen;

		chstat->_freq = (int)wavinfo->nSampleRate;
		chstat->_noteR = note;
		chstat->_noteT = notedef->tnote;
	}

	trackstat_t* pTrack = tracks + track;
	
	chstat->vol = playinfo->vol;
	chstat->vel = playinfo->vel;
	chstat->expr = playinfo->expr;
	chstat->pan = playinfo->pan;
	chstat->pan2 = notedef->pan;
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

static inline void PrepareTrack(int i, int pos)
{
	memset(tracks + i, 0, sizeof(trackstat_t));
	tracks[i].pos = pos;
	tracks[i].playinfo.vol = 64;
	tracks[i].playinfo.vel = 64;
	tracks[i].playinfo.expr = 127;
	tracks[i].playinfo.pan = 64;
	tracks[i].playinfo.pitchb = 0;
	tracks[i].playinfo.pitchr = 2;
	tracks[i].prio = 64;
	tracks[i].a = -1; tracks[i].d = -1; tracks[i].s = -1; tracks[i].r = -1;
}

void PlaySeq(data_t* seq, data_t* bnk, data_t* war)
{
	seqBnk = bnk->data;
	seqWar[0] = war[0].data;
	seqWar[1] = war[1].data;
	seqWar[2] = war[2].data;
	seqWar[3] = war[3].data;

	// Load sequence data
	seqData = (u8*)seq->data + ((u32*)seq->data)[6];
	ntracks = 1;
	
	int pos = 0;

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
}

void StopSeq()
{
	ntracks = 0, seq_bpm = 0;
	int i;
	for (i = 0; i < 16; i ++)
		_NoteStop(i);
}

volatile int seq_bpm = 0;

void track_tick(int n);

void seq_tick()
{
	int i;
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
		track_tick(i);
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
		chstat->reg.TIMER = SOUND_FREQ(ADJUST_FREQ_2(chstat->_freq, chstat->_noteR, chstat->_noteT, info->pitchb, info->pitchr));
	}
}

void track_tick(int n)
{
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
				break;
			}
			case 0xC2: // MASTER VOL
			{
#ifdef LOG_SEQ
				nocashMessage("MASTER VOL");
#endif
				ADSR_mastervolume = SEQ_READ8(track->pos); track->pos ++;
				break;
			}
			case 0xC3: // TRANSPOSE
			case 0xC8: // TIE
			case 0xC9: // PORTAMENTO
			case 0xCA: // MODULATION DEPTH
			case 0xCB: // MODULATION SPEED
			case 0xCC: // MODULATION TYPE
			case 0xCD: // MODULATION RANGE
			case 0xCE: // PORTAMENTO ON/OFF
			case 0xCF: // PORTAMENTO TIME
			case 0xD6: // PRINT VAR
			{
				// TODO
#ifdef LOG_SEQ
				nocashMessage("DUMMY1");
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
				int shouldRepeat = 1;
				if (track->loopcount > 0)
					shouldRepeat = --track->loopcount;
				if (shouldRepeat)
					track->pos = track->looppos;
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
			case 0xE3: // SWEEP PITCH
			{
				// TODO
#ifdef LOG_SEQ
				nocashMessage("DUMMY2");
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
				track->pos --;
				return;
			}
		}
	}
}
