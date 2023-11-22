#if PLATFORM_IOS
#include "iOSNatives.h"

FString GetIOSDocumentsPath()
{
    return FString([[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0] UTF8String]);
}

#endif