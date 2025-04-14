//
// Created by Keijo Länsikunnas on 27.2.2025.
//

#include <stdio.h>
#include <string.h>
#include <cmath>

#include "FreeMono12pt7b.h"
#include "mono_hlsb_test.h"

static const uint8_t raspberry26x32[] =
        {0x0, 0x0, 0xe, 0x7e, 0xfe, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xfe, 0xfe, 0xfc, 0xf8, 0xfc, 0xfe,
         0xfe, 0xff, 0xff,0xff, 0xff, 0xff, 0xfe, 0x7e,
         0x1e, 0x0, 0x0, 0x0, 0x80, 0xe0, 0xf8, 0xfd,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff,0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfd,
         0xf8, 0xe0, 0x80, 0x0, 0x0, 0x1e, 0x7f, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0x7f, 0x1e, 0x0, 0x0,
         0x0, 0x3, 0x7, 0xf, 0x1f, 0x1f, 0x3f, 0x3f,
         0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f, 0x7f, 0x3f,
         0x3f, 0x1f, 0x1f, 0xf, 0x7, 0x3, 0x0, 0x0 };


int main() {

    printf("\nBoot\n");

    mono_hlsb_test display;
    display.fill(0);
    display.text("Hello", 0, 0);
    //mono_vlsb rb(raspberry26x32, 26, 32);
    //display.blit(rb, 20, 20);
    display.rect(15, 15, 35, 45, 1);
    display.line(60, 5, 120, 60, 1);
    display.line(60, 60, 120, 5, 1);
    display.setfont(&FreeMono12pt7b);
    display.text("Free Mono", 10, 100);
    display.show();

    return 0;
}

