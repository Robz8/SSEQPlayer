#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sndcommon.h>

extern bool dsiFix;

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
	if (dsiFix) {
		// Source: gbatek
		// BUG: DSi7 accidently reads from SineTable instead of PitchTable,
		// as workaround for obtaining PitchTable values,
		// one can set "r0=(0..2FFh)-46Ah" on DSi.
		pitch -= 0x46A;
	}
	freq = (u64)basefreq * ((u32)swiGetPitchTable(pitch) + 0x10000);
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
