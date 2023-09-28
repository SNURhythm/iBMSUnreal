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
	void CreateTable(sqlite3* db);

	// Insert ChartMeta
	void Insert(sqlite3* db, UChartMeta& chartMeta);
	TArray<TObjectPtr<UChartMeta>> SelectAll(sqlite3* db);
	TArray<TObjectPtr<UChartMeta>> Search(sqlite3* db, FString& keyword);
	void Delete(sqlite3* db, FString& path);
	void Clear(sqlite3* db);
	void Close(sqlite3* db);
	void BeginTransaction(sqlite3* db);
	void CommitTransaction(sqlite3* db);
private:
	TObjectPtr<UChartMeta> ReadChartMeta(sqlite3_stmt* stmt);
};
