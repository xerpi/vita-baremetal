#include <stddef.h>
#include "lcd.h"
#include "pervasive.h"
#include "spi.h"
#include "i2c.h"
#include "gpio.h"
#include "sysroot.h"
#include "utils.h"

/* TODO: Remove this */
#define SPI_BASE_ADDR	0xE0A00000
#define SPI_REGS(i)	((void *)(SPI_BASE_ADDR + (i) * 0x10000))

#define LCD_BACKLIGHT_I2C_ADDR	0xC8

#define DELAY 0x0D
#define END 0xFF

struct lcd_spi_cmd {
	unsigned char cmd;
	unsigned char size;
	unsigned char data[];
};

struct lcd_i2c_bl_cmd {
	unsigned char reg;
	unsigned char data;
};

static const unsigned char lcd_bl_reset_cmd_1[] = {
	0x01, 0x01,
	0x03, 0x05,
	0x05, 0xFF,
	0x07, 0x72,
	END, END
};

static const unsigned char lcd_bl_reset_cmd_2[] = {
	0x01, 0x11,
	0x03, 0x01,
	0x05, 0xFF,
	0x07, 0x72,
	END, END
};

static const unsigned char lcd_bl_reset_cmd_3[] = {
	0x01, 0x11,
	0x03, 0x01,
	0x04, 0x00,
	0x05, 0xFF,
	0x06, 0x06,
	0x07, 0x72,
	END, END
};

static const unsigned char lcd_bl_reset_cmd_4[] = {
	0x01, 0x11,
	0x03, 0x05,
	0x05, 0xFF,
	0x07, 0x72,
	END, END
};

static const unsigned char lcd_cmd_disp_on_1[] = {
	0x0D, 0x04, 0x11, 0x00, 0x0D, 0x0D,
	0xB0, 0x01, 0x00,
	0xC0, 0x07, 0x03, 0x21, 0x02, 0x23, 0x02, 0x07, 0x07,
	0xB0, 0x01, 0x03,
	0x29, 0x00,
	0x0D, 0x01, 0xFF,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const unsigned char lcd_cmd_disp_on_2[] = {
	0x0D, 0x04, 0x11, 0x00, 0x0D, 0x0D,
	0xB0, 0x01, 0x00,
	0xC0, 0x07, 0x03, 0x1F, 0x02, 0x23, 0x02, 0x07, 0x07,
	0xB0, 0x01, 0x03,
	0x29, 0x00,
	0x0D, 0x01, 0xFF,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const unsigned char lcd_cmd_disp_on_3[] = {
	0x0D, 0x04, 0x11, 0x00, 0x0D, 0x0D,
	0xB0, 0x01, 0x00,
	0xBB, 0x03, 0x08, 0xFF, 0x01,
	0xB0, 0x01, 0x03,
	0x0D, 0x04, 0x29, 0x00, 0x0D, 0x01,
	0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const unsigned char lcd_cmd_colormode_0[] = {
	0xB0, 0x01, 0x04,
	0xC9, 0x01, 0x00,
	0xB0, 0x01, 0x03,
	0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const unsigned char lcd_cmd_colormode_1[] = {
	0xB0, 0x01, 0x04,
	0xC9, 0x01, 0x01,
	0xB0, 0x01, 0x03,
	0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const unsigned char lcd_cmd_unk_0[] = {
	0xB0, 0x01, 0x00,
	0xB8, 0x05, 0x00, 0x1A, 0x18, 0x02, 0x40,
	0xB0, 0x01, 0x03,
	0xFF, 0x00, 0x00
};

static const unsigned char lcd_cmd_unk_1[] = {
	0xB0, 0x01, 0x00,
	0xB8, 0x05, 0x01, 0x1A, 0x18, 0x02, 0x40,
	0xB0, 0x01, 0x03,
	0xFF, 0x00, 0x00
};

static void lcd_spi_write_cmd(const struct lcd_spi_cmd *cmd)
{
	unsigned int i;
	volatile unsigned int *spi_regs = SPI_REGS(2);
	unsigned int tmpbuff = 0;
	unsigned int tmpbuff_size = 0;

	pervasive_clock_enable_spi(2);

	while (spi_regs[0xA])
		spi_regs[0];

	spi_regs[2] = 0x30001;
	spi_regs[4] = 1;

	tmpbuff |= ((rbit(cmd->cmd) >> 24) << 1) << tmpbuff_size;
	tmpbuff_size += 9;

	while (tmpbuff_size > 15) {
		while (spi_regs[0xB] == 0x7F)
			;

		spi_regs[1] = tmpbuff;

		tmpbuff >>= 16;
		tmpbuff_size -= 16;
	}

	for (i = 0; i < cmd->size; i++) {
		unsigned char data = rbit(cmd->data[i]) >> 24;

		tmpbuff |= ((data << 1) | 1) << tmpbuff_size;
		tmpbuff_size += 9;

		while (tmpbuff_size > 15) {
			while (spi_regs[0xB] == 0x7F)
				;

			spi_regs[1] = tmpbuff;

			tmpbuff >>= 16;
			tmpbuff_size -= 16;
		}
	}

	while (tmpbuff_size > 0) {
		tmpbuff |= ((rbit(0) >> 24) << 1) << tmpbuff_size;
		tmpbuff_size += 9;

		while (tmpbuff_size > 15) {
			while (spi_regs[0xB] == 0x7F)
				;

			spi_regs[1] = tmpbuff;

			tmpbuff >>= 16;
			tmpbuff_size -= 16;
		};
	}

	spi_regs[4] = 1;

	while (spi_regs[0xB])
		;

	spi_regs[4] = 0;

	while (spi_regs[4] & 1)
		;
	dsb();

	pervasive_clock_disable_spi(2);
}

static void lcd_spi_read(unsigned char *buffer, unsigned int size, int header_bits)
{
	volatile unsigned int *spi_regs = SPI_REGS(2);
	unsigned int readcnt = 0;
	unsigned int tmp_bits = 0;
	unsigned int tmp = 0;

	pervasive_clock_enable_spi(2);

	while (readcnt < size) {
		while (spi_regs[0xA] == 0)
			;

		tmp = tmp | (spi_regs[0] << tmp_bits);
		tmp_bits = tmp_bits + 16;

		if (header_bits != 0) {
			tmp_bits = tmp_bits - header_bits;
			tmp = tmp >> header_bits;
			header_bits = 0;
			if (tmp_bits == 7)
				continue;
		}

		while (tmp_bits >= 8) {
			buffer[readcnt++] = rbit(tmp & 0xFF) >> 24;
			tmp = tmp >> 8;
			tmp_bits -= 8;
		}
	}

	pervasive_clock_disable_spi(2);
}

static void lcd_spi_write_cmdlist(const struct lcd_spi_cmd *cmdlist)
{
	const struct lcd_spi_cmd *cmd = cmdlist;
	int delay_count = 0;

	while (cmd) {
		if (delay_count <= 0) {
			if (cmd->cmd == END) {
				break;
			} else if (cmd->cmd == DELAY) {
				delay_count = cmd->size - 1;
				cmd++;
			} else {
				lcd_spi_write_cmd(cmd);
				cmd = (struct lcd_spi_cmd *)&cmd->data[cmd->size];
			}
		} else {
			delay_count--;
		}
		delay(16000 / 160);
	}
}

static void lcd_i2c_bl_write_cmdlist(const struct lcd_i2c_bl_cmd *cmdlist)
{
	while (cmdlist->reg != END) {
		i2c_transfer_write(1, LCD_BACKLIGHT_I2C_ADDR, (void *)cmdlist, 2);
		cmdlist++;
	}
}

static void lcd_bl_reset(void)
{
	struct lcd_i2c_bl_cmd *cmdlist;
	unsigned int hw_info = sysroot_get_hw_info() & 0xffff00;
	unsigned int hw_rev = hw_info - 0x804000;

	if (sysroot_is_au_codec_ic_conexant()) {
		cmdlist = (struct lcd_i2c_bl_cmd *)lcd_bl_reset_cmd_3;
	} else {
		if (hw_rev != 0)
			hw_rev = 1;

		if (hw_info > 0x8043ff)
			hw_rev = 0;

		if (hw_rev == 0) {
			if (hw_info < 0x901000)
				cmdlist = (struct lcd_i2c_bl_cmd *)lcd_bl_reset_cmd_2;
			else
				cmdlist = (struct lcd_i2c_bl_cmd *)lcd_bl_reset_cmd_1;
		} else {
			cmdlist = (struct lcd_i2c_bl_cmd *)lcd_bl_reset_cmd_4;
		}
	}

	lcd_i2c_bl_write_cmdlist(cmdlist);
}

static void lcd_display_on(unsigned short supplier_elective_data)
{
	static const int colormode = 0;
	static const int unk = 0;
	struct lcd_spi_cmd *cmdlist_on, *cmdlist_colormode, *cmdlist_unk;

	if (supplier_elective_data < 0x25) {
		cmdlist_on = (struct lcd_spi_cmd *)lcd_cmd_disp_on_2;
	} else {
		if (supplier_elective_data < 0x32)
			cmdlist_on = (struct lcd_spi_cmd *)lcd_cmd_disp_on_1;
		else
			cmdlist_on = (struct lcd_spi_cmd *)lcd_cmd_disp_on_3;
	}

	if (colormode)
		cmdlist_colormode = (struct lcd_spi_cmd *)lcd_cmd_colormode_1;
	else
		cmdlist_colormode = (struct lcd_spi_cmd *)lcd_cmd_colormode_0;

	if (unk)
		cmdlist_unk = (struct lcd_spi_cmd *)lcd_cmd_unk_1;
	else
		cmdlist_unk = (struct lcd_spi_cmd *)lcd_cmd_unk_0;

	lcd_spi_write_cmdlist(cmdlist_on);
	lcd_spi_write_cmdlist(cmdlist_colormode);
	lcd_spi_write_cmdlist(cmdlist_unk);
}

static void lcd_read_ddb(unsigned short *supplier_id,
			 unsigned short *supplier_elective_data)
{
	static const struct lcd_spi_cmd read_ddb_cmd = {
		.cmd = 0xA1,
		.size = 6,
		.data = {0, 0, 0, 0, 0, 0}
	};

	unsigned char ddb[5];

	lcd_spi_write_cmd(&read_ddb_cmd);
	lcd_spi_read(ddb, sizeof(ddb), 9);

	if (supplier_id)
		*supplier_id = ddb[0] | (ddb[1] << 8);
	if (supplier_elective_data)
		*supplier_elective_data = ddb[2] | (ddb[3] << 8);
}

int lcd_init(void)
{
	unsigned short supplier_elective_data;

	spi_init(2);
	pervasive_clock_disable_spi(2);

	gpio_set_port_mode(0, GPIO_PORT_OLED_LCD, GPIO_PORT_MODE_OUTPUT);
	gpio_set_port_mode(1, 7, GPIO_PORT_MODE_OUTPUT);
	gpio_port_set(0, GPIO_PORT_OLED_LCD);
	gpio_port_set(1, 7);

	lcd_read_ddb(NULL, &supplier_elective_data);

	lcd_bl_reset();

	lcd_display_on(supplier_elective_data);

	return 0;
}
