#ifndef STDIO_H
#define STDIO_H

char getc(void);
void putc(char c);

void puts(const char* s);

// This version of gets copies until newline, replacing newline with null char, or until buflen.
// whichever comes first
void gets(char* buf, int buflen);

// Specific logging function, logs the num as hex or binary
void log_uint(uint32_t num, char type);

#endif