// Copyright (C) 2022  ilobilo

#pragma once

#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

// Stubs for fmtlib
typedef size_t FILE;
static FILE *stdout = (FILE*)&stdout;
static FILE *stderr = (FILE*)&stderr;

int fputc(char c, FILE *stream);
int fputs(const char *str, FILE *stream);
int fputws(const wchar_t *str, FILE *stream);

int fprintf(FILE *stream, const char *format, ...);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);

int printf(const char *format, ...);
int vprintf(const char *format, va_list arg);

int sprintf(char *str, const char *format, ...);
int vsprintf(char *str, const char *format, va_list arg);

int snprintf(char *str, size_t count, const char *format, ...);
int vsnprintf(char *str, size_t count, const char *format, va_list arg);

int vasprintf(char **str, const char *format, va_list args);
int asprintf(char **str, const char *format, ...);

int fctprintf(void (*out)(char c, void *extra_arg), void *extra_arg, const char *format, ...);
int vfctprintf(void (*out)(char c, void *extra_arg), void *extra_arg, const char *format, va_list arg);

#ifdef __cplusplus
} // extern "C"
#endif