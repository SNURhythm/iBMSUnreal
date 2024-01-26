#if PLATFORM_ANDROID
#include "AndroidNatives.h"
#include "Android/AndroidJNI.h"
#include "Android/AndroidApplication.h"
FString GetAndroidExternalFilesDir()
{
	JNIEnv* Env = FAndroidApplication::GetJavaEnv();
	jmethodID GetInstalledPakPathMethodID = FJavaWrapper::FindMethod(Env, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_GetExternalStoragePath", "()Ljava/lang/String;", false);
	return FJavaHelper::FStringFromLocalRef(Env, (jstring)FJavaWrapper::CallObjectMethod(Env, FJavaWrapper::GameActivityThis,GetInstalledPakPathMethodID));
}
#endif
