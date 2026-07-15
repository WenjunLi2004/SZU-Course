/*
 * Copyright (c) 2016, Matt Redfearn
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * This library provides a simple implementation of printf and sprintf. I wrote
 * it because I could not find a good BSD licensed printf that may be built in
 * to proprietary software.
 *
 * The formats specifiers supported by this implementation are:
 * 'd' 'u' 'x' 'X' 'c' 's'.
 *
 * Zero padding and field width are also supported, as well as pad right and
 * variable pad width.
 *
 * To use printf() your program should provide an implementation of puutchar()
 * to output a character, for example:
 *
 * void puutchar(char c)
 * {
 *     while(!SERIAL_PORT_EMPTY);
 *     SERIAL_PORT_TX_REGISTER = c;
 * }
 *
 * The code is re-entrant, so it is safe to call from any context, though you
 * will probably get interleaved output. This means that your puutchar()
 * function should also be reentrant.
 */


#ifndef __SIMPLE_PRINTF__
#define __SIMPLE_PRINTF__

/* Platform must supply this function to output a character */
__device__ int puutchar(int c);

__device__ void simple_outpuutchar(char **str, char c);

__device__ int prints(char **out, const char *string, int width, int flags);

__device__ int simple_outputi(char **out, long long i, int base, int sign, int width, int flags, int letbase);


__device__ int simple_vsprintf(char **out, const char *in_format, va_list ap);

__device__ int simple_printf(char *fmt, ...);

__device__ int simple_sprintf(char *buf, const char *fmt, ...);

#endif /* __SIMPLE_PRINTF__ */
