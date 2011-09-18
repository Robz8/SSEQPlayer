#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <sndcommon.h>

static void sound_timer();
static void sndsysMsgHandler(int, void*);

void InstallSoundSys()
{
	/* Power sound on */
	powerOn(POWER_SOUND);
	writePowerManagement(PM_CONTROL_REG, ( readPowerManagement(PM_CONTROL_REG) & ~PM_SOUND_MUTE ) | PM_SOUND_AMP );
	REG_SOUNDCNT = SOUND_ENABLE;
	REG_MASTER_VOLUME = 127;

	/* Install timer */
	timerStart(1, ClockDivider_64, -2728, sound_timer);

	/* Install FIFO */
	fifoSetDatamsgHandler(FIFO_SNDSYS, sndsysMsgHandler, 0);

	/* Clear track-channel assignations */
	register int i;
	for (i = 0; i < 16; i ++)
		ADSR_ch[i].track = -1;
}

static void ADSR_tick();

static void sound_timer()
{
	static volatile int v = 0;

	ADSR_tick();
	
	while (v > 240)
		v -= 240, seq_tick();
	v += seq_bpm;

#ifdef LOG_SEQ
	char tt[20];
	siprintf(tt, "%X", v);
	nocashMessage(tt);
#endif
}

ADSR_stat_t ADSR_ch[16];

volatile int ADSR_mastervolume = 127;

static void ADSR_tickchn(int);

static void ADSR_tick()
{
	register int i;
	for(i = 0; i < 16; i ++)
		ADSR_tickchn(i);
}

static void ADSR_tickchn(int ch)
{
	ADSR_stat_t* chstat = ADSR_ch + ch;
#define AMPL chstat->ampl
#define VOL chstat->vol
#define VEL chstat->vel
#define EXPR chstat->expr
#define PAN chstat->pan
#define PAN2 chstat->pan2
#define REG chstat->reg
#define ATKRATE chstat->a
#define DECRATE chstat->d
#define SUSLEVL chstat->s
#define RELRATE chstat->r
#define SETSTATE(s) chstat->state = (s)

	switch (chstat->state)
	{
		case ADSR_NONE: return;
		case ADSR_SUSTAIN:
			if (!SCHANNEL_ACTIVE(ch))
			{
				SETSTATE(ADSR_NONE);
				chstat->count = 0;
				chstat->track = -1;
				return;
			}
			break;
		case ADSR_START:
			SCHANNEL_CR(ch) = 0;
			SCHANNEL_SOURCE(ch) = REG.SOURCE;
			SCHANNEL_TIMER(ch) = REG.TIMER;
			SCHANNEL_REPEAT_POINT(ch) = REG.REPEAT_POINT;
			SCHANNEL_LENGTH(ch) = REG.LENGTH;
			SCHANNEL_CR(ch) = REG.CR;
			AMPL = -ADSR_THRESHOLD;
			SETSTATE(ADSR_ATTACK);
		case ADSR_ATTACK:
			AMPL = (ATKRATE * AMPL) / 255;
			if (AMPL == 0) SETSTATE(ADSR_DECAY);
			break;
		case ADSR_DECAY:
			AMPL -= DECRATE;
			if (AMPL <= SUSLEVL) AMPL = SUSLEVL, SETSTATE(ADSR_SUSTAIN);
			break;
		case ADSR_RELEASE:
			AMPL -= RELRATE;
			if (AMPL <= -ADSR_THRESHOLD)
			{
//__adsr_release:
				SETSTATE(ADSR_NONE);
				//REG.CR = 0;
				chstat->count = 0;
				chstat->track = -1;
				SCHANNEL_CR(ch) = 0;
				return;
			}
			break;
	}

#define CONV_VOL(a) (CnvSust(a)>>7)
#define SOUND_VOLDIV(n) ((n) << 8)

	int totalvol = CONV_VOL(ADSR_mastervolume);
	totalvol += CONV_VOL(VOL);
	totalvol += CONV_VOL(EXPR);
	totalvol += CONV_VOL(VEL);
	totalvol += AMPL >> 7;
	totalvol += 723;
	if (totalvol < 0) totalvol = 0;

	u32 res = swiGetVolumeTable(totalvol);

	int pan = (int)PAN + (int)PAN2 - 64;
	if (pan < 0) pan = 0;
	if (pan > 127) pan = 127;

	u32 cr = SCHANNEL_CR(ch) &~ (SOUND_VOL(0x7F) | SOUND_VOLDIV(3) | SOUND_PAN(0x7F));
	cr |= SOUND_VOL(res) | SOUND_PAN(pan);
	if (totalvol < (-240 + 723)) cr |= SOUND_VOLDIV(3);
	else if (totalvol < (-120 + 723)) cr |= SOUND_VOLDIV(2);
	else if (totalvol < (-60 + 723)) cr |= SOUND_VOLDIV(1);
	
	SCHANNEL_CR(ch) = cr;
	SCHANNEL_TIMER(ch) = REG.TIMER;

#undef AMPL
#undef VOL
#undef VEL
#undef PAN
#undef PAN2
#undef REG
#undef ATKRATE
#undef DECRATE
#undef SUSLEVL
#undef RELRATE
#undef SETSTATE
}

int ds_freechn()
{
	register int i;
	for(i = 0; i < 16; i ++) if(!SCHANNEL_ACTIVE(i)) return i;
	return -1;
}

int ds_freepsg()
{
	register int i;
	for(i = 8; i < 14; i ++) if(!SCHANNEL_ACTIVE(i)) return i;
	return -1;
}

int ds_freenoise()
{
	register int i;
	for(i = 14; i < 16; i ++) if(!SCHANNEL_ACTIVE(i)) return i;
	return -1;
}

// Adapted from VGMTrans

int CnvAttk(int attk)
{
	const u8 lut[] =
	{
		0x00, 0x01, 0x05, 0x0E, 0x1A, 0x26, 0x33, 0x3F, 0x49, 0x54,
		0x5C, 0x64, 0x6D, 0x74, 0x7B, 0x7F, 0x84, 0x89, 0x8F
	};
	
	return (attk >= 0x6D) ? lut[0x7F-attk] : (0xFF-attk);
}

int CnvFall(int fall)
{
	if      (fall == 0x7F) return 0xFFFF;
	else if (fall == 0x7E) return 0x3C00;
	else if (fall < 0x32)  return ((fall<<1)+1) & 0xFFFF;
	else                   return (0x1E00/(0x7E - fall)) & 0xFFFF;
}

int CnvSust(int sust)
{
	const u16 lut[] =
	{
		0xFD2D, 0xFD2E, 0xFD2F, 0xFD75, 0xFDA7, 0xFDCE, 0xFDEE, 0xFE09, 0xFE20, 0xFE34, 0xFE46, 0xFE57, 0xFE66, 0xFE74,
		0xFE81, 0xFE8D, 0xFE98, 0xFEA3, 0xFEAD, 0xFEB6, 0xFEBF, 0xFEC7, 0xFECF, 0xFED7, 0xFEDF, 0xFEE6, 0xFEEC, 0xFEF3,
		0xFEF9, 0xFEFF, 0xFF05, 0xFF0B, 0xFF11, 0xFF16, 0xFF1B, 0xFF20, 0xFF25, 0xFF2A, 0xFF2E, 0xFF33, 0xFF37, 0xFF3C,
		0xFF40, 0xFF44, 0xFF48, 0xFF4C, 0xFF50, 0xFF53, 0xFF57, 0xFF5B, 0xFF5E, 0xFF62, 0xFF65, 0xFF68, 0xFF6B, 0xFF6F,
		0xFF72, 0xFF75, 0xFF78, 0xFF7B, 0xFF7E, 0xFF81, 0xFF83, 0xFF86, 0xFF89, 0xFF8C, 0xFF8E, 0xFF91, 0xFF93, 0xFF96,
		0xFF99, 0xFF9B, 0xFF9D, 0xFFA0, 0xFFA2, 0xFFA5, 0xFFA7, 0xFFA9, 0xFFAB, 0xFFAE, 0xFFB0, 0xFFB2, 0xFFB4, 0xFFB6,
		0xFFB8, 0xFFBA, 0xFFBC, 0xFFBE, 0xFFC0, 0xFFC2, 0xFFC4, 0xFFC6, 0xFFC8, 0xFFCA, 0xFFCC, 0xFFCE, 0xFFCF, 0xFFD1,
		0xFFD3, 0xFFD5, 0xFFD6, 0xFFD8, 0xFFDA, 0xFFDC, 0xFFDD, 0xFFDF, 0xFFE1, 0xFFE2, 0xFFE4, 0xFFE5, 0xFFE7, 0xFFE9,
		0xFFEA, 0xFFEC, 0xFFED, 0xFFEF, 0xFFF0, 0xFFF2, 0xFFF3, 0xFFF5, 0xFFF6, 0xFFF8, 0xFFF9, 0xFFFA, 0xFFFC, 0xFFFD, 
		0xFFFF, 0x0000
	};
	
	return (sust == 0x7F) ? 0 : -((0x10000-(int)lut[sust]) << 7);
}

void sndsysMsgHandler(int bytes, void* user_data)
{
	sndsysMsg msg;
	fifoGetDatamsg(FIFO_SNDSYS, bytes, (u8*) &msg);

	switch(msg.msg)
	{
		/* The following code must be rethought */
		/*
		case SNDSYS_PLAY:
		{
			int ch = ds_freechn();
			if (ch < 0) goto _play_ret;

			ADSR_stat_t* chstat = ADSR_ch + ch;

			chstat->reg = msg.sndreg;
			chstat->a = CnvAttk(msg.a);
			chstat->d = CnvFall(msg.d);
			chstat->s = CnvSust(msg.s);
			chstat->r = CnvFall(msg.r);
			chstat->vol = msg.vol;
			chstat->vel = msg.vel;
			chstat->expr = 0x7F;
			chstat->pan = msg.pan;
			chstat->state = ADSR_START;

_play_ret:
			fifoSendValue32(FIFO_SNDSYS, (u32) ch);
			return;
		}

		case SNDSYS_STOP:
		{
			ADSR_stat_t* chstat = ADSR_ch + msg.ch;
			chstat->state = ADSR_RELEASE;
			return;
		}
		*/

		case SNDSYS_PLAYSEQ:
		{
			PlaySeq(&msg.seq, &msg.bnk, msg.war);
			return;
		}

		case SNDSYS_STOPSEQ:
		{
			StopSeq();
			return;
		}
	}
}
