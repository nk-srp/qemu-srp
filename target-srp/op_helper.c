/*
 *  SRP helper routines
 *
 *  Copyright (c) 2011 Kevin Hjc
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */
#include "exec.h"
#include "helper.h"

#define SIGNBIT (uint32_t)0x80000000

void raise_exception(int tt)
{
    env->exception_index = tt;
    cpu_loop_exit();
}

//dxw begin
uint32_t HELPER (add_cc)(uint32_t a, uint32_t b)
{
    uint32_t result;
    result = a + b;
    env->SF = env->ZF = result;
    env->CF = result < a;
    env->OF = (a ^ b ^ -1) & (a ^ result);
    return result;
}

uint32_t HELPER(sub_cc)(uint32_t a, uint32_t b)
{
    uint32_t result;
    result = a - b;
    env->SF = env->ZF = result;
    env->CF = a >= b;
    env->OF = (a ^ b) & (a ^ result);
    return result;
}


uint32_t HELPER(rol_cc)(uint32_t x)
{
    env->CF = (x >> 31) & 1;
    return ((uint32_t)x << 1) | ((x >> 31) & 1);
}

uint32_t HELPER(ror_cc)(uint32_t x)
{
    env->CF = x & 1;
    return ((uint32_t)x >> 1) | (x << 31);
 }

uint32_t HELPER(rcl_cc)(uint32_t x)
{
    uint32_t temp = env->CF;
    env->CF = (x >> 31) & 1;
    return ((uint32_t)x << 1) | (temp & 1);
 }

uint32_t HELPER(rcr_cc)(uint32_t x)
{
    uint32_t temp = env->CF;
    env->CF = x & 1;
    return ((uint32_t)x >> 1) |(temp << 31);
}  // dxw end

void HELPER(exception)(uint32_t excp)
{
    env->exception_index = excp;
    cpu_loop_exit();
}
