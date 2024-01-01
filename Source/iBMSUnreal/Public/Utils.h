﻿#pragma once

class FUtils
{
public:
	inline static FString GameName = "iBMSUnreal";
	inline static FString TeamName = "SNURhythm";
	static FString GetDocumentsPath(FString SubPath)
	{
#if PLATFORM_IOS
		return FPaths::Combine(GetIOSDocumentsPath(), SubPath);
#else
		// for other platforms, use Documents
		return FPaths::Combine(FPlatformProcess::UserDir(), TeamName, GameName, SubPath);
#endif
	}
};

class FAsyncGraphTask
	: public FAsyncGraphTaskBase
{
public:

	/** Creates and initializes a new instance. */
	FAsyncGraphTask(ENamedThreads::Type InDesiredThread, TUniqueFunction<void()>&& InFunction)
		: DesiredThread(InDesiredThread)
		, Function(MoveTemp(InFunction))
	{ }

	/** Performs the actual task. */
	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
	{
		Function();
	}

	/** Returns the name of the thread that this task should run on. */
	ENamedThreads::Type GetDesiredThread()
	{
		return DesiredThread;
	}

private:

	/** The thread to execute the function on. */
	ENamedThreads::Type DesiredThread;

	/** The function to execute on the Task Graph. */
	TUniqueFunction<void()> Function;
};

FGraphEventRef AsyncTaskAndReturn(ENamedThreads::Type Thread, TUniqueFunction<void()> Function)
{
	return TGraphTask<FAsyncGraphTask>::CreateTask().ConstructAndDispatchWhenReady(Thread, MoveTemp(Function));
}