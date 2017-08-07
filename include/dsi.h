#ifndef DSI_H
#define DSI_H

enum dsi_bus {
	DSI_BUS_0		= 0,
	DSI_BUS_1		= 1,
	DSI_BUS_OLED_LCD	= DSI_BUS_0,
	DSI_BUS_HDMI		= DSI_BUS_1,
};

void dsi_init(void);
int dsi_get_dimensions_for_vic(unsigned int vic, unsigned int *width, unsigned int *height);
int dsi_get_pixelclock_for_vic(unsigned int vic, unsigned int bpp, unsigned int *pixelclock);
void dsi_enable_bus(enum dsi_bus bus, unsigned int vic);
void dsi_unk(enum dsi_bus bus, unsigned int vic, unsigned int unk);

#endif
