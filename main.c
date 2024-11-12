#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define SAMPLE_RATE 65536
#define BUF_LEN 4096

uint32_t pixels[160*144];

uint8_t vram[0x4000];
uint8_t ram[0x10000];
uint8_t hram[128];
uint8_t gb_io[128];
uint8_t oam[256];
uint8_t ram_bank = 1;
uint8_t r_FF01 = 0;
int LY = 0;
uint16_t rom_bank = 1;
int line_cycles = 0;
uint8_t buttons = 0;
uint8_t button_mode = 0;

uint8_t BGP_colors[64];
uint8_t OGP_colors[64];

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include "audio.h"
#include "renderer.h"

uint8_t *rom;

int CGB_MODE = 0;
int DOUBLE_SPEED = 0;

const uint8_t cycle_lut[256] = {
   1, 3, 2, 2, 1, 1, 2, 1, 5, 2, 2, 2, 1, 1, 2, 1, 1, 3, 2, 2, 1, 1, 2, 1, 3, 2, 2, 2, 1, 1, 2, 1, 2, 3, 2, 2, 1, 1, 2, 1, 2, 2, 2, 2, 1, 1, 2, 1, 2, 3, 2, 2, 3, 3, 3, 1, 2, 2, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 3, 3, 4, 3, 4, 2, 4, 2, 4, 3, 0, 3, 6, 2, 4, 2, 3, 3, 0, 3, 4, 2, 4, 2, 4, 3, 0, 3, 0, 2, 4, 3, 3, 2, 0, 0, 4, 2, 4, 4, 1, 4, 0, 0, 0, 2, 4, 3, 3, 2, 1, 0, 4, 2, 4, 3, 2, 4, 1, 0, 0, 2, 4
};

const uint8_t cycle_lut_cb[256] = {
   2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2
};


static const uint8_t ortab[] = {
    0x80, 0x3f, 0x00, 0xff, 0xbf,
    0xff, 0x3f, 0x00, 0xff, 0xbf,
    0x7f, 0xff, 0x9f, 0xff, 0xbf,
    0xff, 0xff, 0x00, 0x00, 0xbf,
    0x00, 0x00, 0x70,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

struct regs_Struct {
    uint8_t a,b,c,d,e,f,h,l;
    uint16_t pc,sp;
};
struct flags_Struct {
    uint8_t z,n,h,c;
};

struct regs_Struct regs;
struct flags_Struct flags;

void set_flag_reg() {
    flags.z = (regs.f>>7)&1;
    flags.n = (regs.f>>6)&1;
    flags.h = (regs.f>>5)&1;
    flags.c = (regs.f>>4)&1;
}


uint8_t load_flag_reg() {
    return (flags.z<<7)|(flags.n<<6)|(flags.h<<5)|(flags.c<<4);
}

uint8_t readGB(uint16_t addr) {
    //printf("R: %04x\n",addr);
    if (addr & 0x8000) {
        if (addr >= 0x8000 && addr < 0xA000) {
            if (!CGB_MODE) {
                return vram[addr&0x1fff];
            } else {
                return vram[(addr&0x1fff)|((gb_io[0x4F]&1)<<13)];
            }
        }
        if (CGB_MODE) {
            if (addr >= 0xA000 && addr < 0xD000) return ram[addr-0xa000];
            if (addr >= 0xD000 && addr < 0xE000) return ram[(addr-0xa000)+((gb_io[0x70]&7)<<12)];
            if (addr >= 0xE000 && addr < 0xFE00) return ram[(addr-0xd000)+((gb_io[0x70]&7)<<12)];
        } else {
            if (addr >= 0xA000 && addr < 0xE000) return ram[addr-0xa000];
            if (addr >= 0xE000 && addr < 0xFE00) return ram[addr-0xe000+8192];
        }
        if (addr >= 0xFE00 && addr < 0xFF00) return oam[addr&0xff];
        if (addr == 0xFF44) {
            return LY;
        }

        if (addr == 0xFF4D && CGB_MODE) {
            return (gb_io[0x4D]&0x7f)|(DOUBLE_SPEED?0x80:0);
        }
        if (addr == 0xFF00) {
            uint8_t JOYP = button_mode<<4;
            switch (button_mode) {
                case 0: // i think it OR's both button matrixes if both modes are set'
                    JOYP |= (buttons&0x0f);
                    JOYP |= ((buttons>>4)&0x0f);
                    JOYP ^= 0xf;
                    break;
                case 1:
                    JOYP |= 0xf^(buttons&0x0f);
                    break;
                case 2:
                    JOYP |= 0xf^((buttons>>4)&0x0f);
                    break;
                case 3:
                    JOYP |= 0x0f;
                    break;
            }
            return JOYP;
        }
        if (addr == 0xFF69 && CGB_MODE) {
            return BGP_colors[gb_io[0x68]&0x3f];
        }
        if (addr == 0xFF6B && CGB_MODE) {
            return OGP_colors[gb_io[0x6A]&0x3f];
        }
        if (addr >= 0xFF10 && addr < 0xFF40) {
            // "NO BOY! NO DEMO!"
            if (addr >= 0xFF30 && (chans[2].enabled&&chans[2].powered)) return 0xFF;
            else return gb_io[addr&0x7f]|ortab[(addr&0x7f)-0x10];
         }
        if (addr >= 0xFF00 && addr < 0xFF80) return gb_io[addr&0x7f];
        if (addr >= 0xFF80) return hram[addr&0x7f];
        return 0;
    } else {
        if (addr & 0x4000) {
            return rom[(addr&0x3fff)|(rom_bank*0x4000)];
        } else {
            return rom[addr&0x7fff];
        }
    }
}

int32_t DMA_len = 0;
uint8_t DMA_mode = 0;
uint16_t DMA_start_addr = 0;
uint16_t DMA_end_addr = 0;

void writeGB(uint16_t addr, uint8_t val) {
    //printf("W: %04x %02x\n",addr,val);
    if (addr & 0x8000) {
        if (addr >= 0x8000 && addr < 0xA000) {
            if (!CGB_MODE) {
                vram[addr&0x1fff] = val;
            } else {
                // write to designated VRAM bank (0-1)
                vram[(addr&0x1fff)|((gb_io[0x4F]&1)<<13)] = val;
            }
        }
        if (CGB_MODE) {
            if (addr >= 0xA000 && addr < 0xD000) ram[addr-0xa000] = val;
            if (addr >= 0xD000 && addr < 0xE000) ram[(addr-0xa000)+((gb_io[0x70]&7)<<12)] = val;
            if (addr >= 0xE000 && addr < 0xFE00) ram[(addr-0xd000)+((gb_io[0x70]&7)<<12)] = val;
        } else {
            if (addr >= 0xA000 && addr < 0xE000) ram[addr-0xa000] = val;
            if (addr >= 0xE000 && addr < 0xFE00) ram[addr-0xe000+8192] = val;
        }

        if (addr >= 0xFE00 && addr < 0xFF00) oam[addr&0xff] = val;
        if (addr >= 0xFF10 && addr < 0xFF40) { 
            if ((gb_io[0x26]&128)||(addr==0xFF26)) audio_write(addr,val);
            else gb_io[addr&0x7f] = val;
        } else if (addr >= 0xFF00 && addr < 0xFF80) {
            switch (addr&0x7f) {
                case 0x00: { // OAM DMA
                    button_mode = val>>4&3;
                    break;
                }
                case 0x46: { // OAM DMA
                    for (uint8_t pos = 0; pos < 0xA0; pos++) {
                        oam[pos] = readGB(val<<8|pos);
                    }
                    break;
                }

                case 0x70:
                    gb_io[0x70] = (val&7)==0?1:(val&7);
                    break;

                case 0x51:
                case 0x52:
                case 0x53:
                case 0x54: {
                    if (CGB_MODE) {
                        gb_io[addr&0x7f] = val;
                        DMA_start_addr = gb_io[0x52]|(gb_io[0x51]<<8);
                        DMA_start_addr &= ~15;
                        DMA_end_addr = gb_io[0x54]|(gb_io[0x53]<<8);
                        DMA_end_addr &= ~15;
                        DMA_end_addr &= ~(7<<13);
                        DMA_end_addr &= 0x1fff;
                        DMA_end_addr |= 0x8000;
                    } else {
                        gb_io[addr&0x7f] = val;
                    }
                    break;
                }

                case 0x55: { // VRAM DMA length/mode/start
                    if (CGB_MODE) {
                        if (val&128) {
                            // hblank HDMA
                            DMA_len = ((val&127)+1)<<4;
                            DMA_mode = 1;
                        } else {
                            // general purpose DMA
                            DMA_len = ((val&127)+1)<<4;
                            DMA_mode = 0;
                        }
                        gb_io[addr&0x7f] = val;
                    } else {
                        gb_io[addr&0x7f] = val;
                    }
                    break;
                }

                case 0x69: { // BCPD/BGPD
                    if (CGB_MODE) {
                        uint8_t auto_inc = gb_io[0x68]>>7;
                        BGP_colors[gb_io[0x68]&0x3f] = val;
                        gb_io[0x68] = ((gb_io[0x68]+auto_inc)&0x3f)|(gb_io[0x68]&0x80);
                    } else {
                        gb_io[addr&0x7f] = val;
                    }
                    break;
                }

                case 0x6B: { // OCPD/OBPD
                    if (CGB_MODE) {
                        uint8_t auto_inc = gb_io[0x6A]>>7;
                        OGP_colors[gb_io[0x6A]&0x3f] = val;
                        gb_io[0x6A] = ((gb_io[0x6A]+auto_inc)&0x3f)|(gb_io[0x6A]&0x80);
                    } else {
                        gb_io[addr&0x7f] = val;
                    }
                    break;
                }

                case 0x41: // STAT
                    gb_io[addr&0x7f] = (val&0b01111000)|(gb_io[addr&0x7f]&7);
                    break;

                default:
                    gb_io[addr&0x7f] = val;
                    break;
            }
        }
        else if (addr >= 0xFF80) hram[addr&0x7f] = val;
        if (addr == 0xFF01) r_FF01 = val;
        if (addr == 0xFF02 && val == 0x81) putchar(r_FF01);
    }
    if (addr >= 0x2000 && addr < 0x4000) {
        rom_bank = (val&255)==0?1:(val&255);
        //printf("bank: %02x %02d\n",val,val);
    }
}

#define read_byte (readGB(regs.pc++))

void write_r16(uint8_t reg, uint16_t val) {
    switch (reg&3) {
        case 0:
            regs.b = val>>8;
            regs.c = val&0xff;
            break;
        case 1:
            regs.d = val>>8;
            regs.e = val&0xff;
            break;
        case 2:
            regs.h = val>>8;
            regs.l = val&0xff;
            break;
        case 3:
            regs.sp = val;
            break;
    }
}

uint16_t read_r16(uint8_t reg) {
    switch (reg&3) {
        case 0:
            return (regs.b<<8)|regs.c;
            break;
        case 1:
            return (regs.d<<8)|regs.e;
            break;
        case 2:
            return (regs.h<<8)|regs.l;
            break;
        case 3:
            return regs.sp;
            break;
    }
    return 0;
}

uint16_t read_r16mem(uint8_t reg) {
    switch (reg&3) {
        case 0:
            return (regs.b<<8)|regs.c;
            break;
        case 1:
            return (regs.d<<8)|regs.e;
            break;
        case 2: {
            uint16_t r = (regs.h<<8)|regs.l;
            regs.l = (r+1)&0xff;
            regs.h = ((r+1)>>8)&0xff;
            return r;
            break;
        }
        case 3: {
            uint16_t r = (regs.h<<8)|regs.l;
            regs.l = (r-1)&0xff;
            regs.h = ((r-1)>>8)&0xff;
            return r;
            break;
        }
    }
    return 0;
}

void write_r8(uint8_t reg, uint8_t val) {
    switch (reg&7) {
        case 0:
            regs.b = val;
            break;
        case 1:
            regs.c = val;
            break;
        case 2:
            regs.d = val;
            break;
        case 3:
            regs.e = val;
            break;
        case 4:
            regs.h = val;
            break;
        case 5:
            regs.l = val;
            break;
        case 6:
            writeGB((regs.h<<8)|regs.l,val);
            break;
        case 7:
            regs.a = val;
            break;
    }
}

uint8_t read_r8(uint8_t reg) {
    switch (reg&7) {
        case 0:
            return regs.b;
            break;
        case 1:
            return regs.c;
            break;
        case 2:
            return regs.d;
            break;
        case 3:
            return regs.e;
            break;
        case 4:
            return regs.h;
            break;
        case 5:
            return regs.l;
            break;
        case 6:
            return readGB((regs.h<<8)|regs.l);
            break;
        case 7:
            return regs.a;
            break;
    }
    return 0;
}

uint8_t cond_code_GB(uint8_t op) {
    // taken from sameboy :/
    if (((op>>3)&3) == 0) {
        return !flags.z;
    } else if (((op>>3)&3) == 1) {
        return flags.z;
    } else if (((op>>3)&3) == 2) {
        return !flags.c;
    } else {
        return flags.c;
    } 
}

int call_nest = 0;
int cycles = 0;
int do_irq = 1;
int has_halt = 0;
int last_halt = 0;
uint8_t did_HDMA = 0;

void invoke_irq(uint16_t addr) {
    // save PC into stack
    regs.pc--;
    writeGB(--regs.sp, regs.pc >> 8);
    writeGB(--regs.sp, regs.pc & 0xff);

    regs.pc = addr; // set PC to interrupt addr
    cycles += 5;
}

int timer_clocks[4] = {256,4,16,64};

int div_cycles = 0;
int timer_cycles = 0;

uint8_t trig_stat = 0;
uint8_t old_buttons = 0xf;

void gb_irq(int do_coincidence_check) {
    uint8_t LCDC = gb_io[0x40];
    uint8_t LYC = gb_io[0x45];
    uint8_t STAT = gb_io[0x41];
    if (LY == LYC) {
        gb_io[0x41] |= 1<<2; // LYC == LY
        if ((STAT & (1<<6)) && !(trig_stat&(1<<1)) && (line_cycles >= (6>>DOUBLE_SPEED))) {
          gb_io[0x0F] |= 1<<1; // IF |= LCD
          trig_stat |= 1<<1;
        }
    } else {
        gb_io[0x41] &= ~(1<<2); // LYC == LY
    }

    if (LY < 144 && (LCDC & 128)) {
        if (line_cycles < (20<<DOUBLE_SPEED)) {
            gb_io[0x41] = (gb_io[0x41]&(255^3))|2; // mode 2: OAM scan
            if (gb_io[0x41]&(1<<5)&&!(trig_stat&(1<<5))) {
                gb_io[0x0F] |= 1<<1;
                trig_stat |= 1<<5;
            }
        } else if (line_cycles < (62<<DOUBLE_SPEED)) {
            gb_io[0x41] = (gb_io[0x41]&(255^3))|3; // mode 3: LCD rendering
        } else {
            gb_io[0x41] = (gb_io[0x41]&(255^3))|0; // mode 0: hblank
            if (gb_io[0x41]&(1<<3)&&!(trig_stat&(1<<3))) {
                gb_io[0x0F] |= 1<<1;
                trig_stat |= 1<<3;
            }
        }
    } else if (LY < 144) {
        gb_io[0x41] = (gb_io[0x41]&(255^3));
    }

    uint8_t new_buttons = readGB(0xFF00)&0xf;
    uint8_t button_diff = old_buttons^new_buttons;
    if (button_diff != 0) {
      gb_io[0x0F] |= 1<<4; // joypad
    }
    old_buttons = new_buttons;

}

int gb_instr(uint8_t op) {
    //if (regs.pc >= 0x1c0 && regs.pc < 0x200) printf("%d ",line_cycles);


    gb_irq(0);

    if (DMA_len != 0 && CGB_MODE) {
        uint8_t do_hdma = DMA_mode == 1 && (line_cycles >= (62<<DOUBLE_SPEED) || line_cycles <= (20<<DOUBLE_SPEED)) && LY < 144 && (!has_halt) && (!did_HDMA);
        if (do_hdma) did_HDMA = 1;
        if (do_hdma || (DMA_mode == 0)) { 
            regs.pc--;
            for (int i = 0; i < 16; i++) writeGB(DMA_end_addr++,readGB(DMA_start_addr++));
            DMA_len -= 16;
            gb_io[0x55] &= 0x80;
            gb_io[0x55] |= ((DMA_len>>4)-1);
            if (DMA_len == 0) {
                gb_io[0x55] = 0xff;

                gb_io[0x51] = (DMA_start_addr>>8)&0xff;
                gb_io[0x52] = DMA_start_addr&(0xff^15);

                gb_io[0x53] = (DMA_end_addr>>8)&0xff;
                gb_io[0x54] = DMA_end_addr&(0xff^15);
            }
            cycles += 8<<DOUBLE_SPEED;
            return 0;
        }
    }

    if (has_halt || (do_irq && (hram[0x7F]&gb_io[0x0F]&0x1f))) {
        uint8_t can_int = hram[0x7F] & gb_io[0x0F] & 0x1f; // IE & IF


        gb_irq(1);

        if (has_halt && can_int) {
            has_halt = 0;
            op = read_byte;
        } else if (has_halt) {
            regs.pc--;
            cycles += 2; // used to be 1 but CPU usage wasn't the best tbh
            return 0;
        }

        if (do_irq && can_int) {
            do_irq = 0;
            if (can_int & (1<<0)) { // VBLANK
                invoke_irq(0x40);
                gb_io[0x0F] ^= 1<<0;
                op = read_byte;
            } else if (can_int & (1<<1)) { // STAT/LCDC
                invoke_irq(0x48);
                gb_io[0x0F] ^= 1<<1;
                op = read_byte;
            } else if (can_int & (1<<2)) { // TIMER
                invoke_irq(0x50);
                gb_io[0x0F] ^= 1<<2;
                op = read_byte;
            } else if (can_int & (1<<4)) { // TIMER
                invoke_irq(0x60);
                gb_io[0x0F] ^= 1<<4;
                op = read_byte;
            } 
        }
    }

    if (op == 0xCB) {
        uint8_t op = read_byte;
        cycles += cycle_lut_cb[op];
        uint8_t r8 = op&7;
        uint8_t type = op>>3&31;
        uint8_t reg = read_r8(r8);
        if (type == 0) {
            // rlc r8
            uint8_t c = reg>>7&1;
            write_r8(r8,(reg<<1)|c);
            flags.n = 0;
            flags.z = ((reg<<1)|c)==0;
            flags.h = 0;
            flags.c = c;
            return 0;
        }
        if (type == 1) {
            // rrc r8
            uint8_t c = reg&1;
            write_r8(r8,(reg>>1)|(c<<7));
            flags.n = 0;
            flags.z = ((reg>>1)|(c<<7))==0;
            flags.h = 0;
            flags.c = c;
            return 0;
        }
        if (type == 2) {
            // rl r8
            uint8_t c = reg>>7&1;
            write_r8(r8,(reg<<1)|(flags.c));
            flags.n = 0;
            flags.z = (((reg<<1)|(flags.c))&0xff)==0;
            flags.h = 0;
            flags.c = c;
            return 0;
        }
        if (type == 3) {
            // rr r8
            uint8_t c = reg&1;
            write_r8(r8,(reg>>1)|(flags.c<<7));
            flags.n = 0;
            flags.z = ((reg>>1)|(flags.c<<7))==0;
            flags.h = 0;
            flags.c = c;
            return 0;
        }
        if (type == 4) {
            // sla r8
            uint8_t c = reg>>7;
            write_r8(r8,reg<<1);
            flags.n = 0;
            flags.z = ((reg<<1)&0xff)==0;
            flags.h = 0;
            flags.c = c;
            return 0;
        }
        if (type == 5) {
            // sra r8
            uint8_t c = reg&1;
            uint8_t b7 = reg>>7&1;
            write_r8(r8,(reg>>1)|(b7<<7));
            flags.n = 0;
            flags.z = (((reg>>1)|(b7<<7))&0xff)==0;
            flags.h = 0;
            flags.c = c;
            return 0;
        }
        if (type == 6) {
            // swap r8
            uint8_t result = (reg>>4)|((reg&0xf)<<4);
            write_r8(r8,result);
            flags.n = 0;
            flags.z = result==0;
            flags.h = 0;
            flags.c = 0;
            return 0;
        }
        if (type == 7) {
            // srl r8
            uint8_t result = reg>>1;
            write_r8(r8,result);
            flags.n = 0;
            flags.z = result==0;
            flags.h = 0;
            flags.c = reg&1;
            return 0;
        }
        if ((op>>6) == 1) {
            // bit b3, r8
            flags.n = 0;
            flags.z = (reg&(1<<((op>>3)&7)))?0:1;
            flags.h = 1;
            return 0;
        }
        if ((op>>6) == 2) {
            // res b3, r8
            write_r8(r8,reg&(255^(1<<((op>>3)&7))));
            return 0;
        }
        if ((op>>6) == 3) {
            // set b3, r8
            write_r8(r8,reg|(1<<((op>>3)&7)));
            return 0;
        }
        //printf("UNIMPLEMENTED OPCODE: %02x %02x\n",0xCB,op);
        exit(1);
        return 1;
    } else {
        cycles += cycle_lut[op];
        if (op == 0b11110011) {
            // di (NOT IMPLEMENTED)
            do_irq = 0;
            return 0;
        }
        if (op == 0b11111011) {
            // ei (NOT IMPLEMENTED)
            do_irq = 1;
            return 0;
        }

        // BLOCK 0

        if (op == 0b00000000) {
            // NOP (yay!)
            return 0;
        }
        if ((op&0b11001111) == 0b00000001) {
            // ld r16, imm16
            uint16_t imm = read_byte;
            imm |= (read_byte)<<8;
            write_r16(op>>4&3,imm);
            return 0;
        }
        if ((op&0b11001111) == 0b00000010) {
            // ld [r16mem], a
            writeGB(read_r16mem(op>>4&3),regs.a); 
            return 0;
        }
        if ((op&0b11001111) == 0b00001010) {
            // ld a, [r16mem]
            regs.a = readGB(read_r16mem(op>>4&3));  
            return 0;
        }
        if (op == 0b00001000) {
            // ld [imm16], sp
            uint16_t imm = read_byte;
            imm |= (read_byte)<<8;
            writeGB(imm,regs.sp&0xff);
            writeGB(imm+1,(regs.sp>>8)&0xff);
            return 0;
        }

        if ((op&0b11001111) == 0b00000011) {
            // inc r16
            write_r16(op>>4&3,read_r16(op>>4&3)+1);
            return 0;
        }
        if ((op&0b11001111) == 0b00001011) {
            // dec r16
            write_r16(op>>4&3,read_r16(op>>4&3)-1);
            return 0;
        }
        if ((op&0b11001111) == 0b00001001) {
            // add hl, r16
            uint32_t t = read_r16(2)+read_r16(op>>4&3);
            flags.n = 0;
            flags.h = (t^read_r16(op>>4&3)^read_r16(2))&0x1000?1:0;
            flags.c = (t>>16)>0;
            write_r16(2,t);
            return 0;
        }

        if ((op&0b11000111) == 0b00000100) {
            // inc r8
            uint16_t result = read_r8(op>>3&7)+1;
            flags.n = 0;
            flags.z = (result&0xff)==0;
            flags.h = (result&0x0f)==0;
            write_r8(op>>3&7,result&0xff);
            return 0;
        }
        if ((op&0b11000111) == 0b00000101) {
            // dec r8
            uint16_t result = read_r8(op>>3&7)-1;
            flags.n = 1;
            flags.z = (result&0xff)==0;
            flags.h = (result&0x0f)==0x0f;
            write_r8(op>>3&7,result&0xff);
            return 0;
        }

        if ((op&0b11000111) == 0b00000110) {
            // ld r8, imm8
            write_r8(op>>3&7,read_byte);
            return 0;
        }

        if (op == 0b00000111) {
            // rlca
            uint8_t c = regs.a>>7&1;
            regs.a = (regs.a<<1)|c;
            flags.n = 0;
            flags.z = 0;
            flags.h = 0;
            flags.c = c;
            return 0;
        }
        if (op == 0b00001111) {
            // rrca
            uint8_t c = regs.a&1;
            regs.a = (regs.a>>1)|(c<<7);
            flags.n = 0;
            flags.z = 0;
            flags.h = 0;
            flags.c = c;
            return 0;
        }
        if (op == 0b00010111) {
            // rla
            uint8_t c = regs.a>>7&1;
            regs.a = (regs.a<<1)|(flags.c);
            flags.n = 0;
            flags.z = 0;
            flags.h = 0;
            flags.c = c;
            return 0;
        }
        if (op == 0b00011111) {
            // rra
            uint8_t c = regs.a&1;
            regs.a = (regs.a>>1)|(flags.c<<7);
            flags.n = 0;
            flags.z = 0;
            flags.h = 0;
            flags.c = c;
            return 0;
        }
        if (op == 0b00100111) {
            // daa
            // code based off of https://blog.ollien.com/posts/gb-daa/
            uint8_t off1 = 0;
            uint8_t off2 = 0;
            uint8_t c = 0;
            int16_t a = (int16_t)regs.a;
            if (flags.n) {
                if (flags.c) {
                    a -= 0x60;
                }
                if (flags.h) {
                    a -= 0x06;
                }
            } else {
                if (((a&0xff) > 0x99) || flags.c) {
                    flags.c = 1;
                    a += 0x60;
                }
                if (((a&0xf)>0x09) || flags.h) {
                    a += 0x06;
                }
            }
            regs.a = (uint8_t)a&0xff;
            flags.h = 0;
            flags.z = regs.a==0;
            return 0;
        }
        if (op == 0b00101111) {
            // cpl
            regs.a ^= 0xFF;
            flags.n = 1;
            flags.h = 1;
            return 0;
        }
        if (op == 0b00110111) {
            // scf
            flags.c = 1;
            flags.h = 0;
            flags.n = 0;
            return 0;
        }
        if (op == 0b00111111) {
            // ccf
            flags.c ^= 1;
            flags.h = 0;
            flags.n = 0;
            return 0;
        }

        if (op == 0b00011000) {
            // jr imm8
            int8_t off = (int8_t)read_byte;
            regs.pc += off;
            return 0;
        }

        if ((op&0b11100111) == 0b00100000) {
            // jr cond, imm8
            int8_t off = (int8_t)read_byte;
            if (cond_code_GB(op)) regs.pc += off;
            return 0;
        }

        if (op == 0b00010000) {
            // stop
            if ((gb_io[0x4D]&1) && CGB_MODE) {
                DOUBLE_SPEED ^= 1;
                gb_io[0x4D] &= 0xFE;
            }
            read_byte;
            return 0;
        }

        // BLOCK 1
        if (op == 0b01110110) {
            // halt
            has_halt = 1;

            regs.pc--;
            //printf("HALT");
            return 0;
        }
        if ((op&0b11000000) == 0b01000000) {
            // ld r8, r8
            write_r8(op>>3&7,read_r8(op&7));
            return 0;
        }

        // BLOCK 2
        if ((op&0b11111000) == 0b10000000) {
            // add a, r8
            uint8_t r = read_r8(op&7);
            uint8_t a = regs.a;
            uint16_t result = ((uint16_t)r)+((uint16_t)a);
            regs.a = result&0xff;
            flags.z = (result&0xff)==0;
            flags.n = 0;
            flags.h = ((r&0xf)+(a&0xf))>0x0f;
            flags.c = result>0xff;
            return 0;
        }
        if ((op&0b11111000) == 0b10001000) {
            // adc a, r8
            uint8_t r = read_r8(op&7);
            uint8_t a = regs.a;
            uint16_t result = ((uint16_t)r)+((uint16_t)a)+((uint16_t)flags.c);
            regs.a = result&0xff;
            flags.z = (result&0xff)==0;
            flags.n = 0;
            flags.h = ((r&0xf)+(a&0xf)+flags.c)>0x0f;
            flags.c = result>0xff;
            return 0;
        }
        if ((op&0b11111000) == 0b10010000) {
            // sub a, r8
            uint8_t r = read_r8(op&7);
            uint8_t a = regs.a;
            uint16_t result = ((uint16_t)a)-((uint16_t)r);
            regs.a = result&0xff;
            flags.z = (result&0xff)==0;
            flags.n = 1;
            flags.h = (a^r^regs.a)&0x10?1:0;
            flags.c = a<r;
            return 0;
        }
        if ((op&0b11111000) == 0b10011000) {
            // sbc a, r8
            uint8_t r = read_r8(op&7);
            uint8_t a = regs.a;
            uint8_t carry = flags.c;
            uint16_t result = ((uint16_t)a)-((uint16_t)r)-((uint16_t)carry);
            regs.a = result&0xff;
            flags.z = (result&0xff)==0;
            flags.n = 1;
            flags.h = (a^r^regs.a)&0x10?1:0;
            flags.c = result>0xff;
            return 0;
        }
        if ((op&0b11111000) == 0b10100000) {
            // and a, r8
            uint8_t r = read_r8(op&7);
            regs.a &= r;
            flags.z = regs.a==0;
            flags.n = 0;
            flags.h = 1;
            flags.c = 0;
            return 0;
        }
        if ((op&0b11111000) == 0b10101000) {
            // xor a, r8
            uint8_t r = read_r8(op&7);
            regs.a ^= r;
            flags.z = regs.a==0;
            flags.n = 0;
            flags.h = 0;
            flags.c = 0;
            return 0;
        }
        if ((op&0b11111000) == 0b10110000) {
            // or a, r8
            uint8_t r = read_r8(op&7);
            regs.a |= r;
            flags.z = regs.a==0;
            flags.n = 0;
            flags.h = 0;
            flags.c = 0;
            return 0;
        }
        if ((op&0b11111000) == 0b10111000) {
            // cp a, r8
            uint8_t r = read_r8(op&7);
            flags.z = regs.a==r;
            flags.n = 1;
            flags.h = (regs.a&0xf)<(r&0xf);
            flags.c = regs.a<r;
            return 0;
        }

        // BLOCK 3
        if (op == 0b11000110) {
            // add a, imm8
            uint8_t r = read_byte;
            uint8_t a = regs.a;
            uint16_t result = ((uint16_t)r)+((uint16_t)a);
            regs.a = result&0xff;
            flags.z = (result&0xff)==0;
            flags.n = 0;
            flags.h = ((r&0xf)+(a&0xf))>0x0f;
            flags.c = result>0xff;
            return 0;
        }
        if (op == 0b11001110) {
            // adc a, imm8
            uint8_t r = read_byte;
            uint8_t a = regs.a;
            uint16_t result = ((uint16_t)r)+((uint16_t)a)+((uint16_t)flags.c);
            regs.a = result&0xff;
            flags.z = (result&0xff)==0;
            flags.n = 0;
            flags.h = ((r&0xf)+(a&0xf)+flags.c)>0x0f;
            flags.c = result>0xff;
            return 0;
        }
        if (op == 0b11010110) {
            // sub a, imm8
            uint8_t r = read_byte;
            uint8_t a = regs.a;
            uint16_t result = ((uint16_t)a)-((uint16_t)r);
            regs.a = result&0xff;
            flags.z = (result&0xff)==0;
            flags.n = 1;
            flags.h = (a^r^regs.a)&0x10?1:0;
            flags.c = a<r;
            return 0;
        }
        if (op == 0b11011110) {
            // sbc a, imm8
            uint8_t r = read_byte;
            uint8_t a = regs.a;
            uint8_t carry = flags.c;
            uint16_t result = ((uint16_t)a)-((uint16_t)r)-((uint16_t)carry);
            regs.a = result&0xff;
            flags.z = (result&0xff)==0;
            flags.n = 1;
            flags.h = (a^r^regs.a)&0x10?1:0;
            flags.c = result>0xff;
            return 0;
        }
        if (op == 0b11100110) {
            // and a, imm8
            uint8_t r = read_byte;
            regs.a &= r;
            flags.z = regs.a==0;
            flags.n = 0;
            flags.h = 1;
            flags.c = 0;
            return 0;
        }
        if (op == 0b11101110) {
            // xor a, imm8
            uint8_t r = read_byte;
            regs.a ^= r;
            flags.z = regs.a==0;
            flags.n = 0;
            flags.h = 0;
            flags.c = 0;
            return 0;
        }
        if (op == 0b11110110) {
            // or a, imm8
            uint8_t r = read_byte;
            regs.a |= r;
            flags.z = regs.a==0;
            flags.n = 0;
            flags.h = 0;
            flags.c = 0;
            return 0;
        }
        if (op == 0b11111110) {
            // cp a, imm8
            uint8_t r = read_byte;
            flags.z = regs.a==r;
            flags.n = 1;
            flags.h = (regs.a&0xf)<(r&0xf);
            flags.c = regs.a<r;
            return 0;
        }
        if ((op&0b11100111) == 0b11000000) {
            // ret cond
            if (cond_code_GB(op)) {
                regs.pc = readGB(regs.sp++);
                regs.pc |= readGB(regs.sp++)<<8;
                --call_nest;
            }
            return 0;
        }
        if (op == 0b11001001) {
            // ret
            regs.pc = readGB(regs.sp++);
            regs.pc |= readGB(regs.sp++)<<8;
            --call_nest;
            return 0;
        }
        if (op == 0b11011001) {
            // reti (INCOMPLETE)
            do_irq = 1;
            regs.pc = readGB(regs.sp++);
            regs.pc |= readGB(regs.sp++)<<8;
            --call_nest;
            return 0;
        }
        if ((op&0b11100111) == 0b11000010) {
            // jp cond, imm16
            uint16_t imm = read_byte;
            imm |= (read_byte)<<8;
            if (cond_code_GB(op)) regs.pc = imm;
            return 0;
        }
        if (op == 0b11000011) {
            // jp imm16
            uint16_t imm = read_byte;
            imm |= (read_byte)<<8;
            regs.pc = imm;
            return 0;
        }
        if (op == 0b11101001) {
            // jp hl
            regs.pc = (regs.h<<8)|regs.l;
            return 0;
        }
        if ((op&0b11100111) == 0b11000100) {
            // call cond, imm16
            uint16_t imm = read_byte;
            imm |= (read_byte)<<8;
            if (cond_code_GB(op)) {
                writeGB(--regs.sp,regs.pc>>8);
                writeGB(--regs.sp,regs.pc&0xff);
                regs.pc = imm;
                ++call_nest;
            }
            return 0;
        }
        if (op == 0b11001101) {
            // call imm16
            uint16_t imm = read_byte;
            imm |= (read_byte)<<8;
            writeGB(--regs.sp,regs.pc>>8);
            writeGB(--regs.sp,regs.pc&0xff);
            regs.pc = imm;
            ++call_nest;
            return 0;
        }
        if ((op&0b11000111) == 0b11000111) {
            // rst tgt3
            writeGB(--regs.sp,regs.pc>>8);
            writeGB(--regs.sp,regs.pc&0xff);
            regs.pc = op&0x38;
            --call_nest;
            return 0;
        }

        if ((op&0b11001111) == 0b11000001) {
            // pop r16stk
            uint8_t reg = (op>>4)&3;
            uint16_t val = readGB(regs.sp++);
            val |= (readGB(regs.sp++)<<8);
            switch (reg) {
                case 0:
                    regs.b = val>>8;
                    regs.c = val&0xff;
                    break;
                case 1:
                    regs.d = val>>8;
                    regs.e = val&0xff;
                    break;
                case 2:
                    regs.h = val>>8;
                    regs.l = val&0xff;
                    break;
                case 3:
                    regs.a = val>>8;
                    regs.f = val&0xf0;
                    set_flag_reg();
                   break;
            }
            return 0;
        }
        if ((op&0b11001111) == 0b11000101) {
            // push r16stk
            uint8_t reg = (op>>4)&3;
            uint16_t val;
            switch (reg) {
                case 0:
                    val = (regs.b<<8)|regs.c;
                    break;
                case 1:
                    val = (regs.d<<8)|regs.e;
                    break;
                case 2:
                    val = (regs.h<<8)|regs.l;
                    break;
                case 3:
                    val = (regs.a<<8)|(load_flag_reg()&0xf0);
                    break;
            }
            writeGB(--regs.sp,val>>8);
            writeGB(--regs.sp,val&0xff);
            return 0;
        }

        if (op == 0b11100010) {
            // ldh [c], a
            writeGB(0xFF00|regs.c,regs.a);
            return 0;
        }
        if (op == 0b11100000) {
            // ldh [imm8], a
            writeGB(0xFF00|read_byte,regs.a);
            return 0;
        }
        if (op == 0b11101010) {
            // ld [imm16], a
            uint16_t imm = read_byte;
            imm |= (read_byte)<<8;
            writeGB(imm,regs.a);
            return 0;
        }
        if (op == 0b11110010) {
            // ldh a, [c]
            regs.a = readGB(0xFF00|regs.c);
            return 0;
        }
        if (op == 0b11110000) {
            // ldh a, [imm8]
            regs.a = readGB(0xFF00|read_byte);
            return 0;
        }
        if (op == 0b11111010) {
            // ld a, [imm16]
            uint16_t imm = read_byte;
            imm |= (read_byte)<<8;
            regs.a = readGB(imm);
            return 0;
        }

        if (op == 0b11101000) {
            // add sp, imm8
            int8_t off = read_byte;
            flags.z = 0;
            flags.n = 0;
            flags.h = ((off&0xf)+(regs.sp&0xf))>0x0f;
            flags.c = ((off&0xff)+(regs.sp&0xff))>0xff;
            regs.sp += off;
            return 0;
        }
        if (op == 0b11111000) {
            // ld hl, sp + imm8	
            int8_t off = read_byte;
            flags.z = 0;
            flags.n = 0;
            flags.h = ((off&0xf)+(regs.sp&0xf))>0x0f;
            flags.c = ((off&0xff)+(regs.sp&0xff))>0xff;
            write_r16(2,regs.sp + off);
            return 0;
        }
        if (op == 0b11111001) {
            // ld sp, hl
            regs.sp = read_r16(2);
            return 0;
        }


    }
    //printf("UNIMPLEMENTED OPCODE: %02x\n",op);
    exit(1);
    return 1;
}

int emu_audio_read;
int emu_audio_write;
int16_t audio_buffer[BUF_LEN];
int pers[4] = {0,0,0,0};
uint8_t pers_out[4] = {0,0,0,0};
uint8_t wavetable_buffer[256];
uint8_t wavetable_ind = 0;
uint16_t LFSR;

uint8_t noise_div[8] = {8, 16, 32, 48, 64, 80, 96, 112};


void callback(void *udata, uint8_t *stream, int len)
{
    nsamples = len>>1;
    update_square(0);
    update_square(1);
    update_wave();
    update_noise();
    for (int i = 0; i < len>>2; i++) {
        float out = 0.0;
        for (int ch = 0; ch < 4; ch++) {
            uint16_t freq = 2048-chans[ch].freq;
            if (chans[ch].freq == 2047) continue;
            int vol;
            if (ch == 3) {
                if (!(chans[ch].lfsr_reg == 0)) LFSR = 0x7fff;
                chans[ch].lfsr_reg = 0;

                pers[ch] += 64;
                vol = chans[ch].volume * chan_playing(&chans[ch]);
                freq = noise_div[chans[ch].lfsr_div]<<chans[ch].freq;
                if (pers[ch] >= freq) {
                    pers[ch] = 0;

                    uint16_t xor = (LFSR & 1) ^ ((LFSR >> 1) & 1);
                    LFSR = (LFSR >> 1) | (xor << 14);
                    if (chans[ch].lfsr_wide) {
                        LFSR &= ~(1 << 6);
                        LFSR |= xor << 6;
                    }
                    LFSR &= 32767;
                }

                if (LFSR&1) vol = -vol;

                out += ((float)(vol))/10.0;
            } else if (ch != 2) {
                pers[ch] += 16;
                vol = chans[ch].volume * (chan_playing(&chans[ch]) * ((2048 - chans[ch].freq)==0?0:1));
                if (pers[ch] >= freq) {
                    pers[ch] -= freq;
                    pers_out[ch] = (pers_out[ch]+1)&7;
                }
                out += ((float)(chans[ch].duty>>pers_out[ch]&1?vol:-vol))/8.0;
            } else {
                pers[ch] += 32;
                if (pers[ch] >= freq) {
                    pers[ch] -= freq;
                    pers_out[ch] = (pers_out[ch]+1)&255;
                }
                vol = wavetable_buffer[pers_out[ch]]<<1;
                vol = chans[ch].volume == 0 ? 0 : ((vol>>(chans[ch].volume-1))-(16>>(chans[ch].volume-1))) * chan_playing(&chans[2]);
                out += ((float)(vol))/8.0;
            }
        }
        ((int16_t*)stream)[i<<1|0] = (int16_t)(out/16.0*32767.0*vol_l);
        ((int16_t*)stream)[i<<1|1] = (int16_t)(out/16.0*32767.0*vol_r);
    }
}

int get_writeable() {
    int writable = 0;
    if (emu_audio_write <= emu_audio_read) {
        writable = emu_audio_read - emu_audio_write;
    } else {
        writable = BUF_LEN - emu_audio_write + emu_audio_read;
    }
    return writable;
}

int main(int argc, char *argv[]) {
	FILE* file = fopen(argv[1], "rb");
    

    fseek(file,0L,SEEK_END);
    long size = ftell(file);
    rom = (uint8_t*)malloc(size+1);
    fseek(file,0L,SEEK_SET);
    for (long i = 0; i < size; i++) {
        rom[i] = fgetc(file);
    }
    fclose(file);

    CGB_MODE = rom[0x143]>>7&1; // set CGB mode if bit 7 is set

    memset(ram,0,65536);
    memset(hram,0,128);
    memset(gb_io,0,128);
    memset(oam,0,256);
    gb_io[0x07] = 0xF8;
    gb_io[0x40] = 0x91;
    gb_io[0x0F] = 0xE1;

    gb_io[0x47] = 0xFC; // BGP
    gb_io[0x48] = 0xE4; // OBP0
    gb_io[0x49] = 0xE4; // OBP1

    gb_io[0x70] = 1; // OBP1

    gb_io[0x26] = 0x80; // NR52

    ram_bank = 1;
    regs.pc = 0x100;
    regs.sp = 0xFFFE;

    regs.a = 0x01;
    if (CGB_MODE) regs.a = 0x11;

    regs.b = 0x00;
    regs.c = 0x13;
    regs.d = 0x00;
    regs.e = 0xD8;
    regs.h = 0x01;
    regs.l = 0x4D;

    flags.z = 1;
    flags.n = 0;
    flags.h = 0;
    flags.c = 0;

    old_buttons = 0xf;

    do_irq = 1;

    LY = 0;
    int why = 0;
    line_cycles = 0;
    int lcd_cycles = 0;
    int did_render = 0;

    emu_audio_read = 0;
	emu_audio_write = BUF_LEN / 2;

	SDL_AudioSpec wanted;
	wanted.freq = SAMPLE_RATE;
	wanted.format = AUDIO_S16;
	wanted.channels = 2;
	wanted.samples = 512;
	wanted.callback = callback;
	wanted.userdata = &audio_buffer;

	SDL_OpenAudio(&wanted, NULL);
	SDL_PauseAudio(0);

    int audio_cycles_sub = 0;
    int audio_cycles_total = 0;

	SDL_Window *window = SDL_CreateWindow
	(
		"aartGB", SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		160 * 8,
		144 * 8,
		SDL_WINDOW_RESIZABLE | SDL_WINDOW_INPUT_FOCUS
	);

    if (window == NULL) exit(1);

	SDL_SetWindowMinimumSize(window, 160, 144);

	SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (renderer == NULL) exit(1);

	SDL_RenderPresent(renderer);

	/* Use integer scale. */
	SDL_RenderSetLogicalSize(renderer, 160, 144);
	SDL_RenderSetIntegerScale(renderer, 1);

	SDL_Texture *texture = SDL_CreateTexture(renderer,
				    SDL_PIXELFORMAT_BGRA32,
				    SDL_TEXTUREACCESS_STREAMING,
				    160, 144);

    SDL_Event event;
    int quit = 0;
    int frame = 0;
    int time = SDL_GetTicks() + 17;
    buttons = 0;

    DMA_len = 0;
    DMA_mode = 0;

    int did_VBLANK = 0;

    while (!quit) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    quit = 1;
                    break;

                case SDL_KEYDOWN: {
                    switch (event.key.keysym.sym) {
                        case SDLK_x:
                            buttons |= (1<<0);
                            break;
                        case SDLK_z:
                            buttons |= (1<<1);
                            break;
                        case SDLK_LSHIFT:
                            buttons |= (1<<2);
                            break;
                        case SDLK_RETURN:
                            buttons |= (1<<3);
                            break;
                        case SDLK_RIGHT:
                            buttons |= (1<<4);
                            break;
                        case SDLK_LEFT:
                            buttons |= (1<<5);
                            break;
                        case SDLK_UP:
                            buttons |= (1<<6);
                            break;
                        case SDLK_DOWN:
                            buttons |= (1<<7);
                            break;
                    }
                    break;
                }

                case SDL_KEYUP: {
                    switch (event.key.keysym.sym) {
                        case SDLK_x:
                            buttons &= ~(1<<0);
                            break;
                        case SDLK_z:
                            buttons &= ~(1<<1);
                            break;
                        case SDLK_LSHIFT:
                            buttons &= ~(1<<2);
                            break;
                        case SDLK_RETURN:
                            buttons &= ~(1<<3);
                            break;
                        case SDLK_RIGHT:
                            buttons &= ~(1<<4);
                            break;
                        case SDLK_LEFT:
                            buttons &= ~(1<<5);
                            break;
                        case SDLK_UP:
                            buttons &= ~(1<<6);
                            break;
                        case SDLK_DOWN:
                            buttons &= ~(1<<7);
                            break;
                    }
                    break;
                }
            }
        }

        cycles = 0;
        gb_instr(read_byte);

        line_cycles += cycles;
        div_cycles += cycles;
        audio_cycles_total += cycles;
        while (div_cycles >= 64) {
            div_cycles -= 64;
            gb_io[0x04]++;
            uint8_t DIV = gb_io[0x04];
            if ((DIV&(DOUBLE_SPEED?0x7f:0x3f)) == 0) {
                for (int i = 0; i < 32; i++) {
                    uint8_t wav = gb_io[0x30|(i>>1)];
                    if (!(i&1)) {
                        wav >>= 4;
                    }
                    wav &= 0xf;
                    wavetable_buffer[wavetable_ind++] = wav;
                }
            }
            update_freq_ticks(cycles);
        }

        uint8_t IE = hram[0x7F];
        uint8_t IF = gb_io[0x0F];

        uint8_t tac = gb_io[0x07];
        timer_cycles += cycles;
        last_halt += cycles;
        if (tac & 4) {
            while (timer_cycles >= timer_clocks[tac&3]) {
                timer_cycles -= timer_clocks[tac&3];
                gb_io[0x05]++; // TIMA
                if (gb_io[0x05] == 0) {
                    gb_io[0x05] = gb_io[0x06]; // TIMA = TMA
                    gb_io[0x0F] |= 1<<2; // IF |= TIMER
                }
            }
        }

        uint8_t LYC = gb_io[0x45];
        uint8_t STAT = gb_io[0x41];
        uint8_t LCDC = gb_io[0x40];
          
        if (line_cycles >= (6>>DOUBLE_SPEED)) {
          if (LY == 144) {
            if (LCDC & 128 && (!did_VBLANK)) {
                gb_io[0x0F] |= 1<<0; // vblank
            }
            if (STAT&(1<<4) && (!did_VBLANK)) gb_io[0x0F] |= 1<<1;
            gb_io[0x41] = (gb_io[0x41]&(255^3))|1; // mode 1: vblank
            did_VBLANK = 1;
          }
        }

        if (line_cycles >= (114<<DOUBLE_SPEED)) {
          did_VBLANK = 0;
          if (LY == 143) {
              //render_cli();   
		      SDL_UpdateTexture(texture, NULL, &pixels, 160 * sizeof(uint32_t));
		      SDL_RenderClear(renderer);
		      SDL_RenderCopy(renderer, texture, NULL, NULL);
		      SDL_RenderPresent(renderer);
              memset(pixels,0xFF,sizeof(pixels));

              int now = SDL_GetTicks();
              if (time <= now) now = 0;
              else now = time - now;
              SDL_Delay(now);
              time += 17;
              frame = 0;
          }

          did_HDMA = 0;
          did_render = 0;
          trig_stat &= ~((1<<5)|(1<<3)|(1<<1)); // omit hblank from reg

          line_cycles -= 114<<DOUBLE_SPEED;

          LY = (LY+1)%154;
        }


        if ((line_cycles >= (62<<DOUBLE_SPEED)) && (!did_render)) {
          did_render = 1;
          if (LY < 144) {
            if (CGB_MODE) do_scanline_CGB(LY);
            else do_scanline(LY);         
          }
        }

    }

    free(rom);
    return 0;
}
