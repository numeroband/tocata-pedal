#pragma once

#include "IPAddress.h"
#include <cstdio>

#define HEX 0
#define DEC 1

class _serial
{
public:
    void begin(uint32_t baudrate) {}
    void print(const char a[]) { printf("%s", a); }
    void print(char a) { { printf("%c", a); }}
    void print(unsigned char a, int format = DEC) { printf("%u", a); }
    void print(int a, int format = DEC) { printf("%d", a); }
    void print(unsigned int a, int format = DEC) { printf("%u", a); }
    void print(long a, int format = DEC) { printf("%ld", a); }
    void print(unsigned long a, int format = DEC) { printf("%lu", a); }
    void print(double a, int = 2) { printf("%lf", a); }
    void print(struct tm * timeinfo, const char * format = nullptr) {}
    void print(IPAddress) {};

    void println(void) { putchar('\n'); }
    void println(const char a[]) { print(a); println(); }
    void println(char a) { print(a); println(); }
    void println(unsigned char a, int format = DEC) { print(a); println(); }
    void println(int a, int format = DEC) { print(a); println(); }
    void println(unsigned int a, int format = DEC) { print(a); println(); }
    void println(long a, int format = DEC) { print(a); println(); }
    void println(unsigned long a, int format = DEC) { print(a); println(); }
    void println(double a, int format = 2) { print(a); println(); }
    void println(struct tm * timeinfo, const char * format = nullptr) { println(); }
    void println(IPAddress) { println(); }

    operator bool() { return true; }
};

extern _serial Serial;

// #include <inttypes.h>
#include <stdint.h>
typedef uint8_t byte;
#include <pico/time.h>
#include <stdlib.h>

// // avoid strncpy security warning
// #pragma warning(disable:4996)

// #define __attribute__(A) /* do nothing */

// #include <midi_Defs.h>

// float analogRead(int pin)
// {
// 	return 0.0f;
// }

inline unsigned long millis()
{
    return to_ms_since_boot(get_absolute_time());     
}

inline void randomSeed(float)
{
    srand(static_cast<unsigned int>(millis()));
}

inline int random(int min, int max)
{
	return RAND_MAX % std::rand() % (max-min) + min;
}

template <class T> const T& min(const T& a, const T& b) {
    return !(b < a) ? a : b;     // or: return !comp(b,a)?a:b; for version (2)
}

inline void delay(uint32_t ms)
{
    sleep_ms(ms);
}

#define F(x) x
