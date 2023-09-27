// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "sqlite3.h"
#include "Chart.h"
/**
 * 
 */
class IBMSUNREAL_API ChartDBHelper
{
public:
	//Singleton
	ChartDBHelper() {}
	ChartDBHelper(const ChartDBHelper&) {}
	ChartDBHelper& operator=(const ChartDBHelper&) { return *this; }

	static ChartDBHelper& GetInstance()
	{
		// enable multi-threading
		sqlite3_config(SQLITE_CONFIG_MULTITHREAD);
		static ChartDBHelper instance;
		return instance;
	}
	// Connect, return connection
	sqlite3* Connect();

	// CreateTable
	void CreateTable(sqlite3* db);

	// Insert ChartMeta
	void Insert(sqlite3* db, FChartMeta& chartMeta);
	TArray<FChartMeta> SelectAll(sqlite3* db);
	TArray<FChartMeta> Search(sqlite3* db, FString& keyword);
	void Delete(sqlite3* db, FString& path);
	void Clear(sqlite3* db);
	void Close(sqlite3* db);
private:
	FChartMeta ReadChartMeta(sqlite3_stmt* stmt);
};
