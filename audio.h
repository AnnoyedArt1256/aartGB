// code "borrowed" from MiniGBS :/
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define MAX(a, b) ({ typeof(a) _a = (a); typeof(b) _b = (b); _a >  _b ? _a : _b; })
#define MIN(a, b) ({ typeof(a) _a = (a); typeof(b) _b = (b); _a <= _b ? _a : _b; })
#define countof(x) (sizeof(x)/sizeof(*x))

struct chan_len_ctr {
	int   load;
	bool  enabled;
	float counter;
	float inc;
};

struct chan_vol_env {
	int   step;
	bool  up;
	float counter;
	float inc;
};

struct chan_freq_sweep {
	uint16_t freq;
	int   rate;
	bool  up;
	int   shift;
	float counter;
	float inc;
};

static struct chan {
	bool enabled;
	bool powered;
	bool on_left;
	bool on_right;

	int volume;
	int volume_init;

	uint16_t temp_freq;
	uint16_t freq;
	float    freq_counter;
	float    freq_inc;

	int val;
	int note;

	struct chan_len_ctr len;
	struct chan_vol_env env;
	struct chan_freq_sweep sweep;

	// square
	int duty;
	int duty_counter;

	// noise
	uint16_t lfsr_reg;
	bool     lfsr_wide;
	int      lfsr_div;

	// wave
	uint8_t sample;
} chans[4];

#define FREQ 65536.0f

static float* sample_ptr;
static float* sample_end;

static const int duty_lookup[] = { 0x10, 0x30, 0x3C, 0xCF };
static float vol_l, vol_r;
static float audio_rate;
static bool  muted[4]; // not in chan struct to avoid memset(0) across tracks
static bool paused;

static int nsamples;

bool chan_muted(struct chan* c){
	return muted[c-chans] || !c->enabled || !c->powered || !(c->on_left || c->on_right) || !c->volume;
}


int chan_playing(struct chan* c){
	return (c->enabled&&c->powered&&((c->on_left || c->on_right) && c->volume))?1:0;
}


void chan_enable(int i, bool enable){
	chans[i].enabled = enable;

	uint8_t val = (gb_io[0x26] & 0x80)
		| (chans[3].enabled << 3)
		| (chans[2].enabled << 2)
		| (chans[1].enabled << 1)
		| (chans[0].enabled << 0);

	gb_io[0x26] = val;
}

void update_env(struct chan* c){
	c->env.counter += c->env.inc;

	while(c->env.counter > 1.0f){
		if(c->env.step){
			c->volume += c->env.up ? 1 : -1;
			if(c->volume == 0 || c->volume == 15){
				c->env.inc = 0;
			}
			c->volume = MAX(0, MIN(15, c->volume));
		}
		c->env.counter -= 1.0f;
	}
}

void update_len(struct chan* c){
	if(c->len.enabled){
		c->len.counter += c->len.inc;
		if(c->len.counter > 1.0f){
			chan_enable(c - chans, 0);
			c->len.counter = 0.0f;
		}
	}
}

void update_sweep(struct chan* c){
	c->sweep.counter += c->sweep.inc;

	while(c->sweep.counter > 1.0f){
		if(c->sweep.shift){
			uint16_t inc = (c->sweep.freq >> c->sweep.shift);
			if(!c->sweep.up) inc *= -1;

			c->freq = c->sweep.freq + inc;
			if(c->freq > 2047){
				c->enabled = 0;
			} else {
				c->sweep.freq = c->freq;
				c->freq_inc *= 8.0f;
			}
		} else if(c->sweep.rate){
			c->enabled = 0;
		}
		c->sweep.counter -= 1.0f;
	}
}

void update_square(bool ch2){
	struct chan* c = &chans[ch2?1:0];
	if(!c->powered) return;

	c->freq_inc *= 8.0f;

	for(int i = 0; i < nsamples; i+=2){
		update_len(c);

		if(c->enabled){
			update_env(c);
			if(!ch2) update_sweep(c);
		}
	}
}

static uint8_t wave_sample(int pos, int volume){
	uint8_t sample = gb_io[0x30 + pos / 2];
	if(pos & 1){
		sample &= 0xF;
	} else {
		sample >>= 4;
	}
	return volume ? (sample >> ((uint8_t)(volume-1))) : 0;
}

uint8_t wave_sample2(int pos){
	if(pos & 1){
		return gb_io[0x30 + (pos>>1)]&0xf;
	} else {
		return gb_io[0x30 + (pos>>1)]>>4;
	}
}

void update_wave(void){
	struct chan* c = &chans[2];
	if(!c->powered) return;

	//float freq = 4194304.0f / (float)((2048 - c->freq) << 5);

	c->freq_inc *= 16.0f;

	for(int i = 0; i < nsamples; i+=2){
		update_len(c);
	}
}

void update_noise(void){
	struct chan* c = &chans[3];
	if(!c->powered) return;

	float freq = 4194304.0f / (float)((size_t[]){ 8, 16, 32, 48, 64, 80, 96, 112 }[c->lfsr_div] << (size_t)c->freq);

	if(c->freq >= 14){
		c->enabled = false;
	}

	for(int i = 0; i < nsamples; i+=2){
		update_len(c);

		if(c->enabled){
			update_env(c);
		}
	}
}

bool audio_mute(int chan, int val){
	muted[chan-1] = (val != -1) ? val : !muted[chan-1];
	return muted[chan-1];
}

void audio_reset(void){
	memset(chans, 0, sizeof(chans));
	chans[0].val = chans[1].val = -1;
}

void chan_trigger(int i){
	struct chan* c = chans + i;

	chan_enable(i, 1);
	c->volume = c->volume_init;

	// volume envelope
	{
		uint8_t val = gb_io[0x12 + (i*5)];

		c->env.step    = val & 0x07;
		c->env.up      = val & 0x08;
		c->env.inc     = c->env.step ? (64.0f / (float)c->env.step) / FREQ : 8.0f / FREQ;
		c->env.counter = 0.0f;
	}

	// freq sweep
	if(i == 0){
		uint8_t val = gb_io[0x10];

		c->sweep.freq    = c->freq;
		c->sweep.rate    = (val >> 4) & 0x07;
		c->sweep.up      = !(val & 0x08);
		c->sweep.shift   = (val & 0x07);
		c->sweep.inc     = c->sweep.rate ? (128.0f / (float)(c->sweep.rate)) / FREQ : 0;
		c->sweep.counter = nexttowardf(1.0f, 1.1f);
	}

	if(i == 2){ // wave
		c->val = 0;
	} else if(i == 3){ // noise
		c->lfsr_reg = 0xFFFF;
		c->val = -1;
	}
}

void chan_update_len(int i) {
	struct chan* c = chans + i;
	int len_max = i == 2 ? 256 : 64;
	c->len.inc = (256.0f / (float)(len_max - c->len.load)) / FREQ;
	c->len.counter = 0.0f;
}

int freq_ticks[3];

void update_freq_ticks(int cycles) {
    for (int i = 0; i < 3; i++) {
        chans[i].freq = chans[i].temp_freq; 
    }
}

void audio_write(uint16_t addr, uint8_t val){
	int i = (addr - 0xFF10)/5;

	switch(addr){

		case 0xFF12:
		case 0xFF17:
		case 0xFF21: {
			chans[i].volume_init = val >> 4;
			chans[i].powered = val >> 3;

			// "zombie mode" stuff, needed for Prehistorik Man and probably others
			if(chans[i].powered && chans[i].enabled){

				if(chans[i].env.step == 0){
					if(val & 0x08){
						//debug_msg("(zombie vol++)");
						chans[i].volume++;
					} else {
						//debug_msg("(zombie vol+=2)");
						chans[i].volume+=2;
					}
				} else if(chans[i].env.up != (val & 8)) {
					//debug_msg("(zombie swap)");
					chans[i].volume = 16 - chans[i].volume;
				}

				chans[i].volume &= 0x0F;
				chans[i].env.step = val & 0x07;
			}
		    chans[i].env.step    = val & 0x07;
		    chans[i].env.up      = val & 0x08;
		    chans[i].env.inc     = chans[i].env.step ? (64.0f / (float)chans[i].env.step) / FREQ : 8.0f / FREQ;
		    chans[i].env.counter = 0.0f;
		} break;

		case 0xFF1C:
			chans[i].volume = chans[i].volume_init = (val >> 5) & 0x03;
			break;

		case 0xFF11:
		case 0xFF16:
		case 0xFF20:
			chans[i].len.load = val & 0x3f;
			chans[i].duty = duty_lookup[val >> 6];
			chan_update_len(i);
			break;

		case 0xFF1B:
			chans[i].len.load = val;
			chan_update_len(i);
			break;

		case 0xFF13:
		case 0xFF18:
		case 0xFF1D:
            freq_ticks[i] = 0;
			chans[i].temp_freq = (chans[i].temp_freq&0xff00)|val;
			break;

		case 0xFF1A:
			chans[i].powered = val & 0x80;
			chan_enable(i, val & 0x80);
			break;

		case 0xFF14:
		case 0xFF19:
		case 0xFF1E:
            freq_ticks[i] = 0;
			chans[i].temp_freq = (chans[i].temp_freq&0xff)|((val&7)<<8);

		case 0xFF23:
			chans[i].len.enabled = val & 0x40;
			if(val & 0x80){
				chan_trigger(i);
			}
			break;

		case 0xFF22:
			chans[3].freq = val >> 4;
			chans[3].lfsr_wide = (val & 0x08);
			chans[3].lfsr_div = val & 0x07;
			break;

		case 0xFF24:
			vol_l = ((val >> 4) & 0x07) / 7.0f;
			vol_r = (val & 0x07) / 7.0f;
			break;

		case 0xFF25: {
			for(int i = 0; i < 4; ++i){
				chans[i].on_left  = (val >> (4 + i)) & 1;
				chans[i].on_right = (val >> i) & 1;
			}
		} break;
	}

	gb_io[addr&0xff] = val;
}
