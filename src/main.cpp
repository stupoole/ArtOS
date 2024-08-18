#include <float.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


#include "Serial.h"
#include "types.h"
#include "multiboot_header.h"
#include "VideoGraphicsArray.h"
#include "IDT.h"
#include "PIC.h"
#include "Terminal.h"
// #include "stdlib.h"
// #include "malloc.c"

#include <APIC.h>

#include "ACPI.h"
#include "PCI.h"
#include "multiboot2.h"
#include "stdio.h"
#include "CPUID.h"
#include "RTC.h"
#include "PIT.h"
#include "TSC.h"
#include "SMBIOS.h"
#include "EventQueue.h"
#include "string.h"

/* Check if the compiler thinks you are targeting the wrong operating system. */
#if defined(__linux__)
#error "You are not using a cross-compiler, you will most certainly run into trouble"
#endif

/* This tutorial will only work for the 32-bit ix86 targets. */
#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif


template <typename int_like>
void printf(const char* str, int_like key)
{
    auto& terminal = Terminal::get();
    terminal.write(str);

    int l = 0;
    for (; str[l] != 0; l++);
    terminal.write(key, true);
    terminal.newLine();
}

template <typename int_like>
void print_int(int_like val)
{
    auto& terminal = Terminal::get();
    terminal.write(val);
}

void print_string(const char* str)
{
    auto& terminal = Terminal::get();
    terminal.write(str);
}

void print_char(const char c)
{
    auto& terminal = Terminal::get();
    terminal.write(c);
}

template <typename int_like>
void print_hex(const int_like val)
{
    auto& terminal = Terminal::get();
    terminal.write(val, true);
}


struct gdt_info
{
    u16 limit;
    u32 base;
}__attribute__((packed));

struct gdt_entry
{
    u16 limit_low;
    u16 base_low;
    u8 base_middle;
    u8 access;
    u8 granularity;
    u8 base_high;
} __attribute__((packed));


void get_GDT()
{
    gdt_info gdt{};
    asm("sgdt %0" : "=m"(gdt));
    WRITE("GDT limit: ");
    WRITE(gdt.limit, true);
    WRITE(" GDT base: ");
    WRITE(gdt.base, true);
    NEWLINE();

    for (size_t i = 0; i < 8; i++)
    {
        WRITE("GDT entry:");
        WRITE(i);
        WRITE(" data: ");
        [[maybe_unused]] uintptr_t gdt_ptr = static_cast<ptrdiff_t>(gdt.base + (8 * i));
        WRITE(gdt_ptr, true);
        NEWLINE();
    }
}

u16 get_cs()
{
    u16 i;
    asm("mov %%cs,%0" : "=r"(i));
    WRITE("CS: ");
    WRITE(i, true);
    NEWLINE();
    return i;
}

u16 get_ds()
{
    u16 i;
    asm("mov %%ds,%0" : "=r"(i));
    WRITE("DS: ");
    WRITE(i, true);
    NEWLINE();
    return i;
}


extern u32 DATA_CS;
extern u32 TEXT_CS;
extern int setGdt(u32 limit, u32 base);
/**
 *
 */
// VideoGraphicsArray* vgap;
u8 keyboard_modifiers = 0; // caps, ctrl, alt, shift  -> C ! ^ *


extern "C"
void kernel_main(unsigned long magic, unsigned long boot_info_addr)
{
    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) // Should be 0x36d76289
    {
        // Invalid magic
        return;
    }

    if (boot_info_addr & 7)
    {
        // misaligned MBI
        return;
    }

    // Get logging and timestamping running asap
    auto log = Serial();
    RTC rtc;

    // Then load all the boot information into a usable format.
    LOG("Populating boot info.");
    [[maybe_unused]] artos_boot_header* boot_info = multiboot2_populate(boot_info_addr);
    multiboot2_tag_framebuffer_common* frame_info = multiboot2_get_framebuffer();
    full_madt_t* full_madt = populate_madt(multiboot2_get_MADT_table_address());
    [[maybe_unused]] LocalAPIC local_apic(full_madt->madt_stub->local_apic_address);
    [[maybe_unused]] IOAPIC io_apic(full_madt->io_apic.physical_address);

    // then load the rest of the singleton classes.
    WRITE("Mon Jan 01 00:00:00 1970\tLoading singletons...\n");
    EventQueue events;
    VideoGraphicsArray vga(frame_info);
    Terminal terminal;
    PIC::disable_entirely();

    // remap IRQs in APIC
    io_apic.remapIRQ(2, 32); // PIT moved to pin2 on APIC. 0 is taken for something else
    io_apic.remapIRQ(1, 33); // Keyboard
    io_apic.remapIRQ(8, 40); // RTC

    configurePit(2000);

    // local_apic.configure_timer(1024);
    // Configure interrupt tables and enable interrupts.
    IDT idt;

    LOG("Singletons loaded.");


    vga.drawSplash();
    vga.draw();



    terminal.setScale(2);
    vga.draw();
    terminal.write(" Loading Done.\n");

    // FILE* com = fopen("/dev/com1", "w");
    // fprintf(com, "%s\n", "This should print to com0");

    PCI_list();
    [[maybe_unused]] auto IDE_controller = PCIDevice(0, 1, 1);

    LOG("LOADED OS.");


    // Event handler loop.
    LOG("Entering event loop.");
    while (true)
    {
        if (events.pendingEvents())
        {
            auto [type, data] = events.getEvent();
            // LOG("Found event. Type: ", static_cast<int>(type), " lower: ", data.lower_data, " upper: ",data.upper_data);
            switch (type)
            {
            case NULL_EVENT:
                {
                    WRITE("NULL EVENT\n");
                    break;
                }
            case KEY_UP:
                {
                    size_t cin = data.lower_data;
                    char key = key_map[cin];

                    if (key_map[cin] != 0)
                    {
                        switch (key)
                        {
                        case '*': // shift bit 0
                            {
                                keyboard_modifiers &= 0b1110; // not 0100
                                break;
                            }

                        case '^': // ctrl bit 1
                            {
                                keyboard_modifiers &= 0b1101; // not 0010
                                break;
                            }
                        case '!': // alt bit 2
                            {
                                keyboard_modifiers &= 0b1011; // not 0001
                                break;
                            }
                        default:
                            {
                                break;
                            }
                        }
                    }

                    // todo: Add a line buffer and parsing to inputs on enter.
                    // todo: Add an key handler which deals with modifier keys
                    // todo: handle backspace
                    // todo: write an actual terminal class.

                    // WRITE("Key up event in main loop.\n");
                    break;
                }
            case KEY_DOWN:
                {
                    // WRITE("Key down event in main loop: ");
                    size_t cin = data.lower_data;
                    char key = key_map[cin];
                    // WRITE(key);
                    // NEWLINE();
                    if (key_map[cin] != 0)
                    {
                        switch (key)
                        {
                        case '\b': // backspace
                            {
                                terminal.backspace();
                                break;
                            }
                        case '\t': // tab
                            {
                                terminal.write("    ");
                                break;
                            }
                        case '^': // ctrl bit 1
                            {
                                keyboard_modifiers |= 0b0010;
                                break;
                            }
                        case '*': // shift bit 0
                            {
                                keyboard_modifiers |= 0b0001;
                                // print("Shift pressed", keyboard_modifiers);
                                break;
                            }
                        case '!': // alt bit 2
                            {
                                keyboard_modifiers |= 0b0100;
                                break;
                            }
                        case 'H': // Home
                            {
                                // todo: handle home
                                break;
                            }
                        case 'E': // end
                            {
                                // go to end of line
                                break;
                            }
                        case 'U': // up
                            {
                                // probably won't handle up
                                break;
                            }
                        case 'D': // down
                            {
                                // probably won't handle this
                                break;
                            }
                        case '<': // left
                            {
                                // move left
                                break;
                            }
                        case '>': // right
                            {
                                // move right
                                break;
                            }
                        case 'C': // capital C meaning caps lock
                            {
                                keyboard_modifiers ^= 0b1000;
                                break;
                            }
                        default:
                            {
                                bool is_alpha = (key >= 97 && key <= 122);
                                if (keyboard_modifiers & 0b1000) // caps lock enabled
                                    if (is_alpha) // alphanumeric keys get shifted to caps
                                    {
                                        terminal.write(shift_map[cin]);
                                        break;
                                    }
                                if ((keyboard_modifiers & 0b0001)) // shift is down or capslock is on
                                {
                                    terminal.write(shift_map[cin]);
                                    break;
                                }
                                else
                                {
                                    terminal.write(key);
                                }

                                break;
                            }
                        }
                    }
                    break;
                }
            default:
                {
                    WRITE("Unhandled event.\n");
                    WRITE("Type: ");
                    WRITE(static_cast<int>(type));
                    WRITE(" lower: ");
                    WRITE(data.lower_data, true);
                    WRITE(" upper: ");
                    WRITE(data.upper_data, true);
                    NEWLINE();
                    break;
                }
            }
        }
    }
    WRITE("ERROR: Left main loop.");
    asm("hlt");


    // todo: inherit size of _window and colour depth
    // todo: Create string handling to concatenate strings and print them more easily
    // todo: restructure code to have the graphics stuff handled in a different file with only printf handled in
    // main.cpp
    // todo: add data to the data section containing the splash screen
    // Todo: implement user typing from keyboard inputs
    // Todo: automate the build process
}
