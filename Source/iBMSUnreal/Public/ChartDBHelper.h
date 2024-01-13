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
		sqlite3_config(SQLITE_CONFIG_SERIALIZED);
		static ChartDBHelper instance;
		return instance;
	}
	// Connect, return connection
	sqlite3* Connect();

	// CreateTable
	bool CreateChartMetaTable(sqlite3* db);

	// Insert ChartMeta
	bool InsertChartMeta(sqlite3* db, FChartMeta& chartMeta);
	void SelectAllChartMeta(sqlite3* db, TArray<FChartMeta>& chartMetas);
	void SearchChartMeta(sqlite3* db, FString& keyword, TArray<FChartMeta>& chartMetas);
	bool DeleteChartMeta(sqlite3* db, FString& path);
	bool ClearChartMeta(sqlite3* db);
	void Close(sqlite3* db);
	void BeginTransaction(sqlite3* db);
	void CommitTransaction(sqlite3* db);
	bool CreateEntriesTable(sqlite3* db);
	bool InsertEntry(sqlite3* db, FString& path);
	TArray<FString> SelectAllEntries(sqlite3* db);
	bool DeleteEntry(sqlite3* db, FString& path);
	bool ClearEntries(sqlite3* db);

	static FString ToRelativePath(FString& path);
	static FString ToAbsolutePath(FString& path);
private:
	FChartMeta ReadChartMeta(sqlite3_stmt* stmt);

};
