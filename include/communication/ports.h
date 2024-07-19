//
// Created by artypoole on 30/06/24.
//

#ifndef PORTS_H
#define PORTS_H

#include "types.h"

#define PIC1		0x20		/* IO base address for master PIC */
#define PIC2		0xA0		/* IO base address for slave PIC */
#define PIC1_COMMAND	PIC1
#define PIC1_DATA	(PIC1+1)
#define PIC2_COMMAND	PIC2
#define PIC2_DATA	(PIC2+1)
#define PIC_EOI		0x20

#define KEYB_DATA		0x60 // Read only		/* IO base address for keyboard data buffer */
#define KEYB_CONTROL	0x64 // Write only		/* IO base address for keyboard control buffer */

// CMOS ports
#define CMOS_SELECT 0x70 // Write only
#define CMOS_DATA 0x71 // RW

// CMOS Registers
#define CMOS_SECONDS 0x00 // 0-59
#define CMODS_SECONDS_ALARM 0x01
#define CMOS_MINUTES 0x02 // 0-59
#define CMOS_MINUTES_ALARM 0x03
#define CMOS_HOURS 0x04 // 0-24 or 1-12 with highest bit set if after noon
#define CMOS_HOURS_ALARM 0x05
#define CMOS_WEEKDAY 0x06 // 1-7, 1 is sunday
#define CMOS_MONTHDAY 0x07 // 1-31
#define CMOS_MONTH 0x08 // 1-12
#define CMOS_YEAR 0x09 // 0-99
#define CMOS_CENTURY 0x32 // 19-20?  might not work
#define CMOS_STATUS_A 0x8A   // 0x80 bit is "update in progress" flag -- 0x8* fixes the NMI to disabled
#define CMOS_STATUS_B 0x8B // 0x8* fixes the NMI to disabled
#define CMOS_STATUS_C 0x8C // 0x8* fixes the NMI to disabled

/*
0Ah Status Register A (read/write) (usu 26h)
    Bit 7     - (1) time update cycle in progress, data ouputs undefined
                  (bit 7 is read only)
    Bit 6,5,4 - 22 stage divider. 010b - 32.768 Khz time base (default)
    Bit 3-0   - Rate selection bits for interrupt.
                  0000b - none
                  0011b - 122 microseconds (minimum)
                  1111b - 500 milliseconds
                  0110b - 976.562 microseconds (default)
00100110
0Bh Status Register B (read/write)
    Bit 7 - 1 enables cycle update, 0 disables
    Bit 6 - 1 enables periodic interrupt
    Bit 5 - 1 enables alarm interrupt
    Bit 4 - 1 enables update-ended interrupt
    Bit 3 - 1 enables square wave output
    Bit 2 - Data Mode - 0: BCD, 1: Binary
    Bit 1 - 24/12 hour selection - 1 enables 24 hour mode
    Bit 0 - Daylight Savings Enable - 1 enables

0Ch Status Register C (Read only)
    Bit 7 - Interrupt request flag - 1 when any or all of bits 6-4 are
             1 and appropriate enables (Register B) are set to 1. Generates
             IRQ 8 when triggered.
    Bit 6 - Periodic Interrupt flag
    Bit 5 - Alarm Interrupt flag
    Bit 4 - Update-Ended Interrupt Flag
    Bit 3-0 ???

0Dh Status Register D (read only)
    Bit 7 - Valid RAM - 1 indicates batery power good, 0 if dead or
             disconnected.
    Bit 6-0 ???

The rest of the CMOS memory is used to handle system state stuff like maximum ram and number of connected drives etc.

*/



// extern "C"
inline void outb(u16 port, u8 val)
{
    __asm__ volatile ( "outb %b0, %w1" : : "a"(val), "Nd"(port) : "memory");
    /* There's an outb %al, $imm8 encoding, for compile-time constant port numbers that fit in 8b. (N constraint).
     * Wider immediate constants would be truncated at assemble-time (e.g. "i" constraint).
     * The  outb  %al, %dx  encoding is the only option for all other cases.
     * %1 expands to %dx because  port  is a uint16_t.  %w1 could be used if we had the port number a wider C type */
}

// extern "C"
inline u8 inb(u16 port)
{
    u8 ret;
    __asm__ volatile ( "inb %w1, %b0"
                   : "=a"(ret)
                   : "Nd"(port)
                   : "memory");
    return ret;
}


#endif //PORTS_H