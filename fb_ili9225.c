/*
 * FB driver for the ILI9225 LCD Controller
 *
 * Copyright (C) 2018 Reinforce-II
 * Based on codes by Noralf Tronnes
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>

#include "fbtft.h"

#define DRVNAME		"fb_ili9225"
#define WIDTH		176
#define HEIGHT		220

static int init_display(struct fbtft_par *par)
{
	fbtft_par_dbg(DEBUG_INIT_DISPLAY, par, "%s()\n", __func__);

	par->fbtftops.reset(par);

	/* Initialization sequence from ILI9225 Application Notes */

	/* ***********Power On sequence *************** */
	write_reg(par, 0x10, 0x0000); // Set SAP,DSTB,STB
    write_reg(par, 0x11, 0x0000); // Set APON,PON,AON,VCI1EN,VC
    write_reg(par, 0x12, 0x0000); // Set BT,DC1,DC2,DC3
    write_reg(par, 0x13, 0x0000); // Set GVDD
    write_reg(par, 0x14, 0x0000); // Set VCOMH/VCOML voltage
    msleep(20);             // Delay 20 ms

    // Please follow this power on sequence
    write_reg(par, 0x11, 0x0018); // Set APON,PON,AON,VCI1EN,VC
    write_reg(par, 0x12, 0x1121); // Set BT,DC1,DC2,DC3
    write_reg(par, 0x13, 0x0063); // Set GVDD
    write_reg(par, 0x14, 0x3961); // Set VCOMH/VCOML voltage
    write_reg(par, 0x10, 0x0800); // Set SAP,DSTB,STB
    msleep(10);             // Delay 10 ms
    write_reg(par, 0x11, 0x1038); // Set APON,PON,AON,VCI1EN,VC
    msleep(30);             // Delay 30 ms

    write_reg(par, 0x02, 0x0100); // set 1 line inversion

	if ((par->info->var.rotate % 180) != 0) {
		//R01H:SM=0,GS=0,SS=0 (for details,See the datasheet of ILI9225)
		write_reg(par, 0x01, 0x001C); // set the display line number and display direction
		//R03H:BGR=1,ID0=1,ID1=1,AM=1 (for details,See the datasheet of ILI9225)
		write_reg(par, 0x03, 0x1038); // set GRAM write direction .
	}
	else {
		//R01H:SM=0,GS=0,SS=1 (for details,See the datasheet of ILI9225)
		write_reg(par, 0x01, 0x011C); // set the display line number and display direction
		//R03H:BGR=1,ID0=1,ID1=1,AM=0 (for details,See the datasheet of ILI9225)
		write_reg(par, 0x03, 0x1030); // set GRAM write direction.
	}

    write_reg(par, 0x07, 0x0000); // Display off
    write_reg(par, 0x08, 0x0808); // set the back porch and front porch
    write_reg(par, 0x0B, 0x1100); // set the clocks number per line
    write_reg(par, 0x0C, 0x0000); // CPU interface
    write_reg(par, 0x0F, 0x0501); // Set Osc
    write_reg(par, 0x15, 0x0020); // Set VCI recycling
    write_reg(par, 0x20, 0x0000); // RAM Address
    write_reg(par, 0x21, 0x0000); // RAM Address

    //------------------------ Set GRAM area --------------------------------//
    write_reg(par, 0x30, 0x0000);
    write_reg(par, 0x31, 0x00DB);
    write_reg(par, 0x32, 0x0000);
    write_reg(par, 0x33, 0x0000);
    write_reg(par, 0x34, 0x00DB);
    write_reg(par, 0x35, 0x0000);
    write_reg(par, 0x36, 0x00AF);
    write_reg(par, 0x37, 0x0000);
    write_reg(par, 0x38, 0x00DB);
    write_reg(par, 0x39, 0x0000);

    // ---------- Adjust the Gamma 2.2 Curve -------------------//
    write_reg(par, 0x50, 0x0603);
    write_reg(par, 0x51, 0x080D);
    write_reg(par, 0x52, 0x0D0C);
    write_reg(par, 0x53, 0x0205);
    write_reg(par, 0x54, 0x040A);
    write_reg(par, 0x55, 0x0703);
    write_reg(par, 0x56, 0x0300);
    write_reg(par, 0x57, 0x0400);
    write_reg(par, 0x58, 0x0B00);
    write_reg(par, 0x59, 0x0017);

    write_reg(par, 0x0F, 0x0701); // Vertical RAM Address Position
    write_reg(par, 0x07, 0x0012); // Vertical RAM Address Position
    msleep(50);             // Delay 50 ms
    write_reg(par, 0x07, 0x1017); // Vertical RAM Address Position
	return 0;
}

static void set_addr_win(struct fbtft_par *par, int xs, int ys, int xe, int ye)
{
	fbtft_par_dbg(DEBUG_SET_ADDR_WIN, par,
		"%s(xs=%d, ys=%d, xe=%d, ye=%d)\n", __func__, xs, ys, xe, ye);

	if ((par->info->var.rotate % 180) != 0) {
		write_reg(par, 0x38, xe);
		write_reg(par, 0x39, xs);
		write_reg(par, 0x36, ye);
		write_reg(par, 0x37, ys);
		write_reg(par, 0x21, xs);
		write_reg(par, 0x20, ys);
	}
	else {
		write_reg(par, 0x36, xe);
		write_reg(par, 0x37, xs);
		write_reg(par, 0x38, ye);
		write_reg(par, 0x39, ys);
		write_reg(par, 0x20, xs);
		write_reg(par, 0x21, ys);
	}
	write_reg(par, 0x0022); /* Write Data to GRAM */
}


static struct fbtft_display display = {
	.regwidth = 16,
	.width = WIDTH,
	.height = HEIGHT,
	.fbtftops = {
		.init_display = init_display,
		.set_addr_win = set_addr_win,
	},
};
FBTFT_REGISTER_DRIVER(DRVNAME, "ilitek,ili9225", &display);

MODULE_ALIAS("spi:" DRVNAME);
MODULE_ALIAS("platform:" DRVNAME);
MODULE_ALIAS("spi:ili9225");
MODULE_ALIAS("platform:ili9225");

MODULE_DESCRIPTION("FB driver for the ILI9225 LCD Controller");
MODULE_AUTHOR("Reinforce-II");
MODULE_LICENSE("GPL");
