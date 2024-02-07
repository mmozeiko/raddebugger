// Copyright (c) 2024 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

C_LINKAGE_BEGIN

#include "third_party/blake3/blake3_portable.c"

#if defined(__aarch64__) || defined(_M_ARM64)
#include "third_party/blake3/blake3_neon.c"
#endif

#include "third_party/blake3/blake3_dispatch.c"
#include "third_party/blake3/blake3.c"

C_LINKAGE_END

#pragma comment (lib, "blake3")
