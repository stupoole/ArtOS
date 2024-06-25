

#include "terminal.h"

/* Hardware text mode color constants. */


static uint8_t vga_entry_color(vga_color fg, vga_color bg)
{
    return fg | bg << 4;
}

static uint16_t vga_entry(const unsigned char uc, const uint8_t color)
{
    return static_cast<uint16_t>(uc) | static_cast<uint16_t>(color) << 8;
}

size_t strlen(const char* str)
{
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}


void terminal_initialize()
{
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    terminal_buffer = (uint16_t*)0xB8000;
    for (size_t y = 0; y < VGA_HEIGHT; y++)
    {
        for (size_t x = 0; x < VGA_WIDTH; x++)
        {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
}

void terminal_setcolor(const uint8_t color)
{
    terminal_color = color;
}

void terminal_putentryat(const char c, const uint8_t color, const size_t x, const size_t y)
{
    const size_t index = y * VGA_WIDTH + x;
    terminal_buffer[index] = vga_entry(c, color);
}

void terminal_scroll_line()
{
    // for each line from the second line onwards, copy next line into this line
    for (size_t x = 0; x < VGA_WIDTH; x++)
    {
        // All lines move up one
        for (size_t y = 0; y < VGA_HEIGHT-1; y++)
        {
            size_t start_index = (y + 1) * VGA_WIDTH + x;
            size_t end_index = (y) * (VGA_WIDTH) + x;
            terminal_buffer[end_index] = terminal_buffer[start_index];
        }
        // Bottom line is replaced with empty.
        terminal_buffer[VGA_WIDTH * (VGA_HEIGHT - 1) + x] = vga_entry(' ', terminal_color);
    }
    terminal_row-=1;
}

void terminal_new_line()
{
    terminal_column = 0;
    if (++terminal_row >= VGA_HEIGHT)
    {
        terminal_scroll_line();
    }
}


void terminal_putchar(const char c)
{
    // TODO: Implement UEFI frame buffer alternative to support modern
    terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
    if (++terminal_column == VGA_WIDTH)
    {
        terminal_new_line();
    }
}


void terminal_write(const char* data, const size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        const char c = data[i];
        if (c == '\n')
        {
            terminal_new_line();
        }
        else
            terminal_putchar(c);
    }
}

void terminal_writestring(const char* data)
{
    terminal_write(data, strlen(data));
}

void terminal_writechar(const char c)
{
    terminal_write(&c, 1);
}

void terminal_draw_colour_ascii(const char* data, const int8_t* colour, size_t width, size_t height)
{
    for (size_t pixel = 0; pixel < strlen(data); pixel++)
    {
        terminal_putentryat(data[pixel], colour[pixel], pixel / width, pixel % height);
    }
}

size_t string_from_int(long val, char* out_str)
{
    bool const is_negative = val < 0;
    int i = 0;
    int n_digits = 0;
    if (is_negative)
    {
        out_str[i] = '-';
        i++;
        n_digits++;
    }
    auto tmp = val;
    while ((tmp /= 10) != 0) ++n_digits;

    while (i <= n_digits)
    {
        out_str[n_digits - i] = static_cast<char>(val % 10 + 48);
        val /= 10;
        i++;
    }
    out_str[i] = '\0';

    return i;
}