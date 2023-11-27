#pragma once

#ifdef __cplusplus
extern "C" {
#endif
#include <stdatomic.h>
	int transcode(const char* inPath, const char* outPath, void* cancel);
#ifdef __cplusplus
}
#endif