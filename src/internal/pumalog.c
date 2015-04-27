#include "internal/pumalog.h"

#include <stdarg.h>
#include <string.h>
#include <stdio.h>

void pumalogf(const char* format, ...)
{
	const char prefix[] = "[PUMA LOG] ";
	size_t formatLen = strlen(format);
	size_t length = formatLen + (sizeof(prefix) / sizeof(char));
	char logFormat[length];
	strcpy(logFormat, prefix);
	strncat(logFormat, format, formatLen);

	va_list args;
	va_start(args, format);
	vfprintf(stdout, logFormat, args);
	va_end(args);
}

void pumawarnf(const char* format, ...)
{
	const char prefix[] = "[PUMA WARNING] ";
	size_t formatLen = strlen(format);
	size_t length = formatLen + (sizeof(prefix) / sizeof(char));
	char logFormat[length];
	strcpy(logFormat, prefix);
	strncat(logFormat, format, formatLen);

	va_list args;
	va_start(args, format);
	vfprintf(stdout, logFormat, args);
	va_end(args);
}

void pumaerrorf(const char* format, ...)
{
	const char prefix[] = "[PUMA ERROR] ";
	size_t formatLen = strlen(format);
	size_t length = formatLen + (sizeof(prefix) / sizeof(char));
	char logFormat[length];
	strcpy(logFormat, prefix);
	strncat(logFormat, format, formatLen);

	va_list args;
	va_start(args, format);
	vfprintf(stderr, logFormat, args);
	va_end(args);
}