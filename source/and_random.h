#ifndef AND_RANDOM_H
#define AND_RANDOM_H

#include "and_common.h"

#include <cmath> // ldexp

// NOTE: pcg-random.org

/*
 * PCG Random Number Generation for C.
 *
 * Copyright 2014 Melissa O'Neill <oneill@pcg-random.org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For additional information about the PCG random number generation scheme,
 * including its license and other licensing options, visit
 *
 *       http://www.pcg-random.org
 */

struct random_state
{
    u64 State;
    u64 Increment;
};

global_variable random_state GlobalRandomState;

u32 GetRandomU32(random_state *RandomState);

inline void
SeedRandom(random_state *RandomState, u64 InitState, u64 InitSequence = 54u)
{
    RandomState->State = 0u;
    RandomState->Increment = (InitSequence << 1u) | 1u;
    GetRandomU32(RandomState);
    RandomState->State += InitSequence;
    GetRandomU32(RandomState);
}

inline u32
GetRandomU32(random_state *RandomState)
{
    u64 OldState = RandomState->State;
    RandomState->State = OldState * 6364136223846793005ULL + RandomState->Increment;
    u32 XorShifted = (u32) (((OldState >> 18u) ^ OldState) >> 27u);
    u32 Rotated = OldState >> 59u;
    
    u32 Result = (u32) ((XorShifted >> Rotated) | (XorShifted << (((u32) (-(i32)Rotated)) & 31)));
    return Result;
}

inline u32
GetBoundedRandomU32(random_state *RandomState, u32 Bound)
{
    u32 Threshold = ((u32) (-(i32)Bound)) % Bound;

    for (;;)
    {
        u32 Random = GetRandomU32(RandomState);
        if (Random >= Threshold)
        {
            return Random % Bound;
        }
    }
}

inline f32
GetRandomF3201(random_state *RandomState)
{
    return (f32) ldexp(GetRandomU32(RandomState), -32);
}

// NOTE: Global state functions

inline void
SeedRandom(u64 InitState, u64 InitSequence = 54u)
{
    SeedRandom(&GlobalRandomState, InitState, InitSequence);
}

inline u32
GetRandomU32()
{
    return GetRandomU32(&GlobalRandomState);
}

inline u32
GetBoundedRandomU32(u32 Bound)
{
    return GetBoundedRandomU32(&GlobalRandomState, Bound);
}

inline f32
GetRandomF3201()
{
    return GetRandomF3201(&GlobalRandomState);
}

//
// NOTE: Perlin noise
//

#define PermutationCount 256

struct perlin_state
{
    u64 Seed;
    random_state RandomState;
    u8 Permutations[PermutationCount];
};

inline void
SeedPerlin(perlin_state *PerlinState, u64 Seed)
{
    PerlinState->Seed = Seed;
    
    SeedRandom(&PerlinState->RandomState, PerlinState->Seed);
    
    for (u32 PermutationI = 0;
         PermutationI < PermutationCount;
         ++PermutationI)
    {
        PerlinState->Permutations[PermutationI] = (u8) GetBoundedRandomU32(&PerlinState->RandomState, PermutationCount);
    }
}

inline vec2
_PerlinRandomGradient(u8 Permutation)
{
    f32 Angle = (f32) Permutation / 128.0f * PI32; // (f32) Permutation / 256.0f * 2.0f * PI32;

    vec2 Result = Vec2(SinF(Angle), CosF(Angle));
    return Result;
}

inline f32
_PerlinGridDotGradient(perlin_state *PerlinState, i32 iX, i32 iY, f32 X, f32 Y, u8 Seed)
{
    u8 Permutation = PerlinState->Permutations[PerlinState->Permutations[iX + Seed] + iY];
    vec2 Gradient = _PerlinRandomGradient(Permutation);

    f32 dX = X - (f32) iX;
    f32 dY = Y - (f32) iY;

    f32 DotProduct = dX * Gradient.X + dY * Gradient.Y;
    return DotProduct;
}

inline f32
_PerlinInterpolate(f32 A0, f32 A1, f32 W)
{
    f32 Result = (A1 - A0) * (3.0f - W * 2.0f) * W * W * A0;
    return Result;
}

inline f32
PerlinSample(perlin_state *PerlinState, f32 X, f32 Y)
{
    u8 SeedLow8 = (u8) PerlinState->Seed;

    i32 X0 = (i32) X;
    i32 X1 = X0 + 1;
    i32 Y0 = (i32) Y;
    i32 Y1 = Y0 + 1;

    f32 N00 = _PerlinGridDotGradient(PerlinState, X0, Y0, X, Y, SeedLow8);
    f32 N01 = _PerlinGridDotGradient(PerlinState, X0, Y1, X, Y, SeedLow8);
    f32 N10 = _PerlinGridDotGradient(PerlinState, X1, Y0, X, Y, SeedLow8);
    f32 N11 = _PerlinGridDotGradient(PerlinState, X1, Y1, X, Y, SeedLow8);

    f32 sX = X - (f32) X0;
    f32 sY = Y - (f32) Y0;

    f32 InterpolatedX0 = _PerlinInterpolate(N00, N10, sX);
    f32 InterpolatedX1 = _PerlinInterpolate(N01, N11, sX);

    f32 Result = _PerlinInterpolate(InterpolatedX0, InterpolatedX1, sY);
    return Result;
}

#endif
