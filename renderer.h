uint8_t internal_WY = 0;
uint32_t gb_pal[4] = {0xffffff, 0x888888, 0x555555, 0x000000};

void do_scanline(uint8_t LY) {
    uint8_t LYC = gb_io[0x45];
    uint8_t STAT = gb_io[0x41];
    uint8_t LCDC = gb_io[0x40];

    if (LY == 0) internal_WY = 0;
    if (LCDC & 128) {
        if (LCDC & 1) { // if bg is enabled
            uint8_t ypos = LY + gb_io[0x42]; // LY + SCY
            uint16_t tilebase = (LCDC&(1<<4))?0x8000:0x8800;
            uint16_t bgmap = (LCDC&(1<<3))?0x9C00:0x9800;
            bgmap += (ypos>>3)*32; // tilemap is 32x32
            ypos &= 7;
            uint8_t xpos = gb_io[0x43]; // SCX
            uint16_t cur_tile_addr;
            for (uint8_t x = 0; x < 160; x++) {
                uint8_t final_x = xpos+x;
                uint16_t final_map = bgmap|(final_x>>3);
                if ((final_x&7) == 0 || x == 0) {
                    uint8_t tile = vram[final_map&0x1fff];
                    if (!(LCDC&(1<<4))) tile = (tile+0x80)&0xff; // LCDC.4
                    cur_tile_addr = tilebase +
                                    (tile<<4)+(ypos<<1);
                }
                uint8_t pixel_val = 0;
                pixel_val = vram[cur_tile_addr&0x1fff|0]>>(7-(final_x&7))&1;
                pixel_val |= (vram[cur_tile_addr&0x1fff|1]>>(7-(final_x&7))&1)<<1;
                pixel_val = (gb_io[0x47]>>(pixel_val<<1))&3;
                pixels[LY*160+x] = gb_pal[pixel_val&3];
            }   
        }

        if ((LCDC & (1<<5)) && (LY >= gb_io[0x4A])) { // if window is enabled
            uint8_t ypos = internal_WY; // LY - WY
            uint16_t tilebase = (LCDC&(1<<4))?0x8000:0x8800;
            uint16_t bgmap = (LCDC&(1<<6))?0x9C00:0x9800;
            bgmap += (ypos>>3)*32; // tilemap is 32x32
            ypos &= 7;
            int16_t WX = gb_io[0x4B]-7;
            uint16_t cur_tile_addr;
            for (int16_t x = WX; x < 160; x++) {
                uint8_t final_x = x-WX;
                uint16_t final_map = bgmap+(final_x>>3);
                if ((final_x&7) == 0) {
                    uint8_t tile = vram[final_map&0x1fff];
                    if (!(LCDC&(1<<4))) tile = (tile+0x80)&0xff; // LCDC.4
                    cur_tile_addr = tilebase +
                                    (tile<<4)+(ypos<<1);
                }
                uint8_t pixel_val = 0;
                pixel_val = vram[cur_tile_addr&0x1fff|0]>>(7-(final_x&7))&1;
                pixel_val |= (vram[cur_tile_addr&0x1fff|1]>>(7-(final_x&7))&1)<<1;
                pixel_val = (gb_io[0x47]>>(pixel_val<<1))&3;
                pixels[LY*160+x] = gb_pal[pixel_val&3];
           }
           if (WX < 159) internal_WY++;
        }


        if (LCDC & (1<<1)) { // if OAM is enabled
            uint8_t rendered_sprites = 0;
            for (uint8_t sprite = 0; sprite < 40; sprite++) {
                if (rendered_sprites == 10) break; // 10 sprites per line
                
                uint8_t rendered_this_sprite = 0;
                int16_t ypos = oam[sprite<<2|0];   
                uint8_t xpos = oam[sprite<<2|1];   
                uint8_t tile_ind = oam[sprite<<2|2];   
                uint8_t attr = oam[sprite<<2|3];   

                if (xpos == 0 || xpos >= 168) continue;
                if (LY < (ypos-16)) continue;
                ypos = LY-(ypos-16);
                if ((LCDC&(1<<2)) && ypos >= 16) continue;
                if ((!(LCDC&(1<<2))) && ypos >= 8) continue;
                if (attr&(1<<6)) // y-flip
                    ypos = (15>>(LCDC&(1<<2)?0:1)) - ypos;

                uint8_t x_flip = attr&(1<<5);
                uint8_t obj_pal = gb_io[0x48+(attr>>4&1)];

                if (LCDC&(1<<2)) tile_ind &= 0xfe;
                uint16_t cur_tile_addr = (tile_ind<<4)+(ypos<<1);
                if (ypos < 8) {
                    int16_t final_x;
                    for (uint8_t x = 0; x < 8; x++) {
                        uint8_t pixel_val = 0;
                        pixel_val = vram[cur_tile_addr&0x1fff|0]>>(7-x)&1;
                        pixel_val |= (vram[cur_tile_addr&0x1fff|1]>>(7-x)&1)<<1;
                        if (x_flip) final_x = xpos-x-1;
                        else final_x = x+xpos-8;
                        rendered_this_sprite = 1;
                        if (final_x < 0 || final_x >= 160) {
                            continue;
                        }
                        if (!pixel_val) continue; // color 0 is transparent
                        pixel_val = (obj_pal>>(pixel_val<<1))&3; // set OBP palette
                        if ((attr&(1<<7)) && ((gb_io[0x47]&3) != (3-(pixels[LY*160+final_x]>>2&3)))) // OBJ priority    
                            continue;
                        pixels[LY*160+final_x] = gb_pal[pixel_val&3];
                    }
                }

                if ((LCDC&(1<<2)) && (ypos >= 8)) { // 8x16 sprites
                    cur_tile_addr |= 1<<4;
                    int16_t final_x;
                    for (uint8_t x = 0; x < 8; x++) {
                        uint8_t pixel_val = 0;
                        pixel_val = vram[cur_tile_addr&0x1fff|0]>>(7-x)&1;
                        pixel_val |= (vram[cur_tile_addr&0x1fff|1]>>(7-x)&1)<<1;
                        if (x_flip) final_x = xpos-x-1;
                        else final_x = x+xpos-8;
                        rendered_this_sprite = 1;
                        if (final_x < 0 || final_x >= 160) {
                            continue;
                        }
                        if (!pixel_val) continue; // color 0 is transparent
                        pixel_val = (obj_pal>>(pixel_val<<1))&3; // set OBP palette
                        if ((attr&(1<<7)) && ((gb_io[0x47]&3) != (3-(pixels[LY*160+final_x]>>2&3)))) // OBJ priority    
                            continue;
                        pixels[LY*160+final_x] = gb_pal[pixel_val&3];
                    }
                }
                rendered_sprites += rendered_this_sprite;
            }
        }
    }
}


void do_scanline_CGB(uint8_t LY) {
    uint8_t LYC = gb_io[0x45];
    uint8_t STAT = gb_io[0x41];
    uint8_t LCDC = gb_io[0x40];

    uint8_t pixel_buffer[160];

    memset(pixel_buffer,0x00,160);

    if (LY == 0) internal_WY = 0;
    if (LCDC & 128) {
        if (1) { // if bg is enabled
            uint8_t ypos = LY + gb_io[0x42]; // LY + SCY
            uint16_t tilebase = (LCDC&(1<<4))?0x8000:0x8800;
            uint16_t bgmap = (LCDC&(1<<3))?0x9C00:0x9800;
            bgmap += (ypos>>3)*32; // tilemap is 32x32
            ypos &= 7;
            uint8_t xpos = gb_io[0x43]; // SCX
            uint16_t cur_tile_addr;
            uint8_t pal_ind;
            uint8_t attr;
            for (uint8_t x = 0; x < 160; x++) {
                uint8_t final_x = xpos+x;
                uint16_t final_map = bgmap|(final_x>>3);
                if ((final_x&7) == 0 || x == 0) {
                    attr = vram[final_map&0x1fff|0x2000];
                    pal_ind = (attr&7)<<3;
                    uint16_t vram_or = (vram[final_map&0x1fff|0x2000]>>3&1)<<13;
                    uint8_t tile = vram[final_map&0x1fff];
                    uint8_t final_y = ypos;
                    if (attr&(1<<6)) final_y = 7-final_y;
                    if (!(LCDC&(1<<4))) tile = (tile+0x80)&0xff; // LCDC.4
                    cur_tile_addr = tilebase +
                                    (tile<<4)+(final_y<<1);
                    cur_tile_addr |= vram_or;
                }
                uint8_t pixel_val = 0;
                uint8_t tile_x = final_x&7;
                if (!(attr&(1<<5))) tile_x = 7-tile_x;
                pixel_val = (vram[cur_tile_addr&0x3fff|0]>>tile_x)&1;
                pixel_val |= ((vram[cur_tile_addr&0x3fff|1]>>tile_x)&1)<<1;
                pixel_buffer[x] = pixel_val&3;
                if (pixel_val > 0 && (attr&128)) pixel_buffer[x] |= 4;

                uint16_t final_raw_col = BGP_colors[pal_ind|(pixel_val&3)<<1|0];
                final_raw_col |= BGP_colors[pal_ind|(pixel_val&3)<<1|1]<<8;
                uint32_t final_col = (final_raw_col&0x1f)<<19;
                final_col |= ((final_raw_col>>5)&0x1f)<<11;
                final_col |= ((final_raw_col>>10)&0x1f)<<3;
                pixels[LY*160+x] = final_col;
            }   
        }

        if ((LCDC & (1<<5)) && (LY >= gb_io[0x4A])) { // if window is enabled
            uint8_t ypos = internal_WY; // LY - WY
            uint16_t tilebase = (LCDC&(1<<4))?0x8000:0x8800;
            uint16_t bgmap = (LCDC&(1<<6))?0x9C00:0x9800;
            bgmap += (ypos>>3)*32; // tilemap is 32x32
            ypos &= 7;
            int16_t WX = gb_io[0x4B]-7;
            uint16_t cur_tile_addr;
            uint8_t pal_ind;
            uint8_t attr;
            for (int16_t x = WX; x < 160; x++) {
                uint8_t final_x = x-WX;
                uint16_t final_map = bgmap+(final_x>>3);
                if ((final_x&7) == 0) {
                    attr = vram[final_map&0x1fff|0x2000];
                    pal_ind = (attr&7)<<3;
                    uint16_t vram_or = (vram[final_map&0x1fff|0x2000]>>3&1)<<13;
                    uint8_t tile = vram[final_map&0x1fff];
                    uint8_t final_y = ypos;
                    if (attr&(1<<6)) final_y = 7-final_y;
                    if (!(LCDC&(1<<4))) tile = (tile+0x80)&0xff; // LCDC.4
                    cur_tile_addr = tilebase +
                                    (tile<<4)+(final_y<<1);
                    cur_tile_addr |= vram_or;
                }
                uint8_t pixel_val = 0;
                uint8_t tile_x = final_x&7;
                if (!(attr&(1<<5))) tile_x = 7-tile_x;
                pixel_val = (vram[cur_tile_addr&0x3fff|0]>>tile_x)&1;
                pixel_val |= ((vram[cur_tile_addr&0x3fff|1]>>tile_x)&1)<<1;
                pixel_buffer[x] = pixel_val&3;
                if (pixel_val > 0 && (attr&128)) pixel_buffer[x] |= 4;

                uint16_t final_raw_col = BGP_colors[pal_ind|(pixel_val&3)<<1|0];
                final_raw_col |= BGP_colors[pal_ind|(pixel_val&3)<<1|1]<<8;
                uint32_t final_col = (final_raw_col&0x1f)<<19;
                final_col |= ((final_raw_col>>5)&0x1f)<<11;
                final_col |= ((final_raw_col>>10)&0x1f)<<3;
                pixels[LY*160+x] = final_col;
           }
           if (WX < 159) internal_WY++;
        }


        if (LCDC & (1<<1)) { // if OAM is enabled
            uint8_t rendered_sprites = 0;
            for (int8_t sprite = 0; sprite < 40; sprite++) {
                if (rendered_sprites == 10) break; // 10 sprites per line
                
                uint8_t rendered_this_sprite = 0;
                int16_t ypos = oam[sprite<<2|0];   
                uint8_t xpos = oam[sprite<<2|1];   
                uint8_t tile_ind = oam[sprite<<2|2];   
                uint8_t attr = oam[sprite<<2|3];   

                if (xpos == 0 || xpos >= 168) continue;
                if (LY < (ypos-16)) continue;
                ypos = LY-(ypos-16);
                if ((LCDC&(1<<2)) && ypos >= 16) continue;
                if ((!(LCDC&(1<<2))) && ypos >= 8) continue;
                if (attr&(1<<6)) // y-flip
                    ypos = (15>>(LCDC&(1<<2)?0:1)) - ypos;

                uint8_t x_flip = attr&(1<<5);
                uint8_t pal_ind = (attr&7)<<3;

                uint16_t vram_or = (attr>>3&1)<<13;

                if (LCDC&(1<<2)) tile_ind &= 0xfe;
                uint16_t cur_tile_addr = (tile_ind<<4)+(ypos<<1);
                if (ypos < 8) {
                    int16_t final_x;
                    for (uint8_t x = 0; x < 8; x++) {
                        uint8_t pixel_val = 0;
                        pixel_val = vram[cur_tile_addr&0x1fff|0|vram_or]>>(7-x)&1;
                        pixel_val |= (vram[cur_tile_addr&0x1fff|1|vram_or]>>(7-x)&1)<<1;
                        if (x_flip) final_x = xpos-x-1;
                        else final_x = x+xpos-8;
                        rendered_this_sprite = 1;
                        if (final_x < 0 || final_x >= 160) {
                            continue;
                        }
                        if (!pixel_val) continue; // color 0 is transparent

                        if (LCDC & 1) { // BG-OBJ priority
                            if ((attr&(1<<7)) && ((pixel_buffer[final_x]&3) != 0)) // OBJ priority    
                            continue;
                            if (pixel_buffer[final_x]&4) continue; // BG priority
                        }

                        if (pixel_buffer[final_x]&128) continue; // OBJ priority again

                        uint16_t final_raw_col = OGP_colors[pal_ind|(pixel_val&3)<<1|0];
                        final_raw_col |= OGP_colors[pal_ind|(pixel_val&3)<<1|1]<<8;
                        uint32_t final_col = (final_raw_col&0x1f)<<19;
                        final_col |= ((final_raw_col>>5)&0x1f)<<11;
                        final_col |= ((final_raw_col>>10)&0x1f)<<3;
                        pixel_buffer[final_x] |= 128;
                        pixels[LY*160+final_x] = final_col;
                    }
                }

                if ((LCDC&(1<<2)) && (ypos >= 8)) { // 8x16 sprites
                    cur_tile_addr |= 1<<4;
                    int16_t final_x;
                    for (uint8_t x = 0; x < 8; x++) {
                        uint8_t pixel_val = 0;
                        pixel_val = vram[cur_tile_addr&0x1fff|0|vram_or]>>(7-x)&1;
                        pixel_val |= (vram[cur_tile_addr&0x1fff|1|vram_or]>>(7-x)&1)<<1;
                        if (x_flip) final_x = xpos-x-1;
                        else final_x = x+xpos-8;
                        rendered_this_sprite = 1;
                        if (final_x < 0 || final_x >= 160) {
                            continue;
                        }
                        if (!pixel_val) continue; // color 0 is transparent

                        if (LCDC & 1) { // BG-OBJ priority
                            if ((attr&(1<<7)) && ((pixel_buffer[final_x]&3) != 0)) // OBJ priority    
                            continue;
                            if (pixel_buffer[final_x]&4) continue; // BG priority
                        }

                        if (pixel_buffer[final_x]&128) continue; // OBJ priority again

                        uint16_t final_raw_col = OGP_colors[pal_ind|(pixel_val&3)<<1|0];
                        final_raw_col |= OGP_colors[pal_ind|(pixel_val&3)<<1|1]<<8;
                        uint32_t final_col = (final_raw_col&0x1f)<<19;
                        final_col |= ((final_raw_col>>5)&0x1f)<<11;
                        final_col |= ((final_raw_col>>10)&0x1f)<<3;
                        pixel_buffer[final_x] |= 128;
                        pixels[LY*160+final_x] = final_col;
                    }
                }
                rendered_sprites += rendered_this_sprite;
            }
        }
    }
}
