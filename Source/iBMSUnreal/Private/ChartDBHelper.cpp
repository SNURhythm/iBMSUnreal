// Fill out your copyright notice in the Description page of Project Settings.


#include "ChartDBHelper.h"


sqlite3* ChartDBHelper::Connect() {
	IFileManager& FileManager = IFileManager::Get();
	// use iOS Document Directory
#if PLATFORM_IOS
	// mkdir "BMS"
	FString Directory = FPaths::Combine(FPaths::RootDir(), ".db/");
	FileManager.MakeDirectory(*Directory);
#else
	// use Project/BMS. Note that this would not work on packaged build, so we need to make it configurable
	FString Directory = FPaths::Combine(FPaths::ProjectDir(), ".db/");
	FileManager.MakeDirectory(*Directory);
#endif
	
	FString pathRel = FPaths::Combine(Directory, "chart.db");
	FString path = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*pathRel);
	UE_LOG(LogTemp, Log, TEXT("DB Path: %s"), *path);
	UE_LOG(LogTemp, Log, TEXT("DB PathRel: %s"), *pathRel);
	sqlite3* db;
	int rc;
	rc = sqlite3_open(TCHAR_TO_UTF8(*path), &db);
	sqlite3_busy_timeout(db, 1000);
	if (rc) {
		UE_LOG(LogTemp, Error, TEXT("Can't open database: %s"), UTF8_TO_TCHAR(sqlite3_errmsg(db)));
		sqlite3_close(db);
		return nullptr;
	}
	return db;
}
void ChartDBHelper::Close(sqlite3* db) {
	sqlite3_close(db);
}
void ChartDBHelper::BeginTransaction(sqlite3* db)
{
	sqlite3_exec(db, "BEGIN", nullptr, nullptr, nullptr);
}
void ChartDBHelper::CommitTransaction(sqlite3* db)
{
	sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
}
void ChartDBHelper::CreateTable(sqlite3* db) {
	auto query = "CREATE TABLE IF NOT EXISTS chart_meta ("
		"path       TEXT primary key,"
		"md5        TEXT not null,"
		"sha256     TEXT not null,"
		"title      TEXT,"
		"subtitle   TEXT,"
		"genre      TEXT,"
		"artist     TEXT,"
		"sub_artist  TEXT,"
		"folder     TEXT,"
		"stage_file  TEXT,"
		"banner     TEXT,"
		"back_bmp    TEXT,"
		"preview    TEXT,"
		"level      REAL,"
		"difficulty INTEGER,"
		"total     REAL,"
		"bpm       REAL,"
		"max_bpm     REAL,"
		"min_bpm     REAL,"
		"length     INTEGER,"
		"rank      INTEGER,"
		"player    INTEGER,"
		"keys     INTEGER,"
		"total_notes INTEGER,"
		"total_long_notes INTEGER,"
		"total_scratch_notes INTEGER,"
		"total_backspin_notes INTEGER"
		")";
	char* errMsg;
	int rc = sqlite3_exec(db, query, nullptr, nullptr, &errMsg);
	if (rc != SQLITE_OK) {
		UE_LOG(LogTemp, Error, TEXT("SQL error while creating table: %s"), UTF8_TO_TCHAR(errMsg));
		sqlite3_free(errMsg);
	}


}

void ChartDBHelper::Insert(sqlite3* db, UChartMeta& chartMeta) {
	auto query = "REPLACE INTO chart_meta ("
		"path,"
		"md5,"
		"sha256,"
		"title,"
		"subtitle,"
		"genre,"
		"artist,"
		"sub_artist,"
		"folder,"
		"stage_file,"
		"banner,"
		"back_bmp,"
		"preview,"
		"level,"
		"difficulty,"
		"total,"
		"bpm,"
		"max_bpm,"
		"min_bpm,"
		"length,"
		"rank,"
		"player,"
		"keys,"
		"total_notes,"
		"total_long_notes,"
		"total_scratch_notes,"
		"total_backspin_notes"
		") VALUES("
		"@path,"
		"@md5,"
		"@sha256,"
		"@title,"
		"@subtitle,"
		"@genre,"
		"@artist,"
		"@sub_artist,"
		"@folder,"
		"@stage_file,"
		"@banner,"
		"@back_bmp,"
		"@preview,"
		"@level,"
		"@difficulty,"
		"@total,"
		"@bpm,"
		"@max_bpm,"
		"@min_bpm,"
		"@length,"
		"@rank,"
		"@player,"
		"@keys,"
		"@total_notes,"
		"@total_long_notes,"
		"@total_scratch_notes,"
		"@total_backspin_notes"
		")";
	sqlite3_stmt* stmt;
	int rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
	if (rc != SQLITE_OK) {
		UE_LOG(LogTemp, Error, TEXT("SQL error while preparing statement to insert a chart: %s"), UTF8_TO_TCHAR(sqlite3_errmsg(db)));
		sqlite3_close(db);
		return;
	}
	sqlite3_bind_text(stmt, 1, TCHAR_TO_UTF8(*chartMeta.BmsPath), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, TCHAR_TO_UTF8(*chartMeta.MD5), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 3, TCHAR_TO_UTF8(*chartMeta.SHA256), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 4, TCHAR_TO_UTF8(*chartMeta.Title), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 5, TCHAR_TO_UTF8(*chartMeta.SubTitle), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 6, TCHAR_TO_UTF8(*chartMeta.Genre), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 7, TCHAR_TO_UTF8(*chartMeta.Artist), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 8, TCHAR_TO_UTF8(*chartMeta.SubArtist), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 9, TCHAR_TO_UTF8(*chartMeta.Folder), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 10, TCHAR_TO_UTF8(*chartMeta.StageFile), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 11, TCHAR_TO_UTF8(*chartMeta.Banner), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 12, TCHAR_TO_UTF8(*chartMeta.BackBmp), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 13, TCHAR_TO_UTF8(*chartMeta.Preview), -1, SQLITE_TRANSIENT);
	sqlite3_bind_double(stmt, 14, chartMeta.PlayLevel);
	sqlite3_bind_int(stmt, 15, chartMeta.Difficulty);
	sqlite3_bind_double(stmt, 16, chartMeta.Total);
	sqlite3_bind_double(stmt, 17, chartMeta.Bpm);
	sqlite3_bind_double(stmt, 18, chartMeta.MaxBpm);
	sqlite3_bind_double(stmt, 19, chartMeta.MinBpm);
	sqlite3_bind_int64(stmt, 20, chartMeta.PlayLength);
	sqlite3_bind_int(stmt, 21, chartMeta.Rank);
	sqlite3_bind_int(stmt, 22, chartMeta.Player);
	sqlite3_bind_int(stmt, 23, chartMeta.KeyMode);
	sqlite3_bind_int(stmt, 24, chartMeta.TotalNotes);
	sqlite3_bind_int(stmt, 25, chartMeta.TotalLongNotes);
	sqlite3_bind_int(stmt, 26, chartMeta.TotalScratchNotes);
	sqlite3_bind_int(stmt, 27, chartMeta.TotalBackSpinNotes);
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		UE_LOG(LogTemp, Error, TEXT("SQL error while inserting a chart: %s"), UTF8_TO_TCHAR(sqlite3_errmsg(db)));
		sqlite3_close(db);
		return;
	}
	sqlite3_finalize(stmt);



}

TArray<TObjectPtr<UChartMeta>> ChartDBHelper::SelectAll(sqlite3* db) {
	auto query = "SELECT "
		"path,"
		"md5,"
		"sha256,"
		"title,"
		"subtitle,"
		"genre,"
		"artist,"
		"sub_artist,"
		"folder,"
		"stage_file,"
		"banner,"
		"back_bmp,"
		"preview,"
		"level,"
		"difficulty,"
		"total,"
		"bpm,"
		"max_bpm,"
		"min_bpm,"
		"length,"
		"rank,"
		"player,"
		"keys,"
		"total_notes,"
		"total_long_notes,"
		"total_scratch_notes,"
		"total_backspin_notes"
		" FROM chart_meta";
	sqlite3_stmt* stmt;
	int rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
	if (rc != SQLITE_OK) {
		UE_LOG(LogTemp, Error, TEXT("SQL error while getting all charts: %s"), UTF8_TO_TCHAR(sqlite3_errmsg(db)));
		sqlite3_close(db);
		return TArray<TObjectPtr<UChartMeta>>();
	}
	TArray<TObjectPtr<UChartMeta>> chartMetas;
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		auto chartMeta = ReadChartMeta(stmt);
		chartMetas.Add(chartMeta);
	}
	sqlite3_finalize(stmt);
	return chartMetas;
}

TArray<TObjectPtr<UChartMeta>> ChartDBHelper::Search(sqlite3* db, FString& text) {
	auto query = "SELECT "
		"path,"
		"md5,"
		"sha256,"
		"title,"
		"subtitle,"
		"genre,"
		"artist,"
		"sub_artist,"
		"folder,"
		"stage_file,"
		"banner,"
		"back_bmp,"
		"preview,"
		"level,"
		"difficulty,"
		"total,"
		"bpm,"
		"max_bpm,"
		"min_bpm,"
		"length,"
		"rank,"
		"player,"
		"keys,"
		"total_notes,"
		"total_long_notes,"
		"total_scratch_notes,"
		"total_backspin_notes"
		" FROM chart_meta WHERE rtrim(title || ' ' || subtitle || ' ' || artist || ' ' || sub_artist || ' ' || genre) LIKE @text GROUP BY sha256";
	sqlite3_stmt* stmt;
	int rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
	if (rc != SQLITE_OK) {
		UE_LOG(LogTemp, Error, TEXT("SQL error while searching for charts: %s"), UTF8_TO_TCHAR(sqlite3_errmsg(db)));
		sqlite3_close(db);
		return TArray<TObjectPtr<UChartMeta>>();
	}
	sqlite3_bind_text(stmt, 1, TCHAR_TO_UTF8(*("%" + text + "%")), -1, SQLITE_TRANSIENT);

	TArray<TObjectPtr<UChartMeta>> chartMetas;
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		auto chartMeta = ReadChartMeta(stmt);
		chartMetas.Add(chartMeta);
	}
	sqlite3_finalize(stmt);
	return chartMetas;
}

void ChartDBHelper::Delete(sqlite3* db, FString& path) {
	auto query = "DELETE FROM chart_meta WHERE path = @path";
	sqlite3_stmt* stmt;
	int rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
	if (rc != SQLITE_OK) {
		UE_LOG(LogTemp, Error, TEXT("SQL error while preparing statement to delete a chart: %s"), UTF8_TO_TCHAR(sqlite3_errmsg(db)));
		sqlite3_close(db);
		return;
	}
	sqlite3_bind_text(stmt, 1, TCHAR_TO_UTF8(*path), -1, SQLITE_TRANSIENT);
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		UE_LOG(LogTemp, Error, TEXT("SQL error while deleting a chart: %s"), UTF8_TO_TCHAR(sqlite3_errmsg(db)));
		sqlite3_close(db);
		return;
	}
	sqlite3_finalize(stmt);
}

void ChartDBHelper::Clear(sqlite3* db) {
	auto query = "DELETE FROM chart_meta";
	sqlite3_stmt* stmt;
	int rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
	if (rc != SQLITE_OK) {
		UE_LOG(LogTemp, Error, TEXT("SQL error while clearing: %s"), UTF8_TO_TCHAR(sqlite3_errmsg(db)));
		sqlite3_close(db);
		return;
	}
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		UE_LOG(LogTemp, Error, TEXT("SQL error while clearing: %s"), UTF8_TO_TCHAR(sqlite3_errmsg(db)));
		sqlite3_close(db);
		return;
	}
	sqlite3_finalize(stmt);
}

TObjectPtr<UChartMeta> ChartDBHelper::ReadChartMeta(sqlite3_stmt* stmt) {
	int idx = 0;
	const TObjectPtr<UChartMeta> chartMeta = NewObject<UChartMeta>();
	chartMeta->BmsPath = UTF8_TO_TCHAR(sqlite3_column_text(stmt, idx++));
	chartMeta->MD5 = UTF8_TO_TCHAR(sqlite3_column_text(stmt, idx++));
	chartMeta->SHA256 = UTF8_TO_TCHAR(sqlite3_column_text(stmt, idx++));
	chartMeta->Title = UTF8_TO_TCHAR(sqlite3_column_text(stmt, idx++));
	chartMeta->SubTitle = UTF8_TO_TCHAR(sqlite3_column_text(stmt, idx++));
	chartMeta->Genre = UTF8_TO_TCHAR(sqlite3_column_text(stmt, idx++));
	chartMeta->Artist = UTF8_TO_TCHAR(sqlite3_column_text(stmt, idx++));
	chartMeta->SubArtist = UTF8_TO_TCHAR(sqlite3_column_text(stmt, idx++));
	chartMeta->Folder = UTF8_TO_TCHAR(sqlite3_column_text(stmt, idx++));
	chartMeta->StageFile = UTF8_TO_TCHAR(sqlite3_column_text(stmt, idx++));
	chartMeta->Banner = UTF8_TO_TCHAR(sqlite3_column_text(stmt, idx++));
	chartMeta->BackBmp = UTF8_TO_TCHAR(sqlite3_column_text(stmt, idx++));
	chartMeta->Preview = UTF8_TO_TCHAR(sqlite3_column_text(stmt, idx++));
	chartMeta->PlayLevel = sqlite3_column_double(stmt, idx++);
	chartMeta->Difficulty = sqlite3_column_int(stmt, idx++);
	chartMeta->Total = sqlite3_column_double(stmt, idx++);
	chartMeta->Bpm = sqlite3_column_double(stmt, idx++);
	chartMeta->MaxBpm = sqlite3_column_double(stmt, idx++);
	chartMeta->MinBpm = sqlite3_column_double(stmt, idx++);
	chartMeta->PlayLength = sqlite3_column_int64(stmt, idx++);
	chartMeta->Rank = sqlite3_column_int(stmt, idx++);
	chartMeta->Player = sqlite3_column_int(stmt, idx++);
	chartMeta->KeyMode = sqlite3_column_int(stmt, idx++);
	chartMeta->TotalNotes = sqlite3_column_int(stmt, idx++);
	chartMeta->TotalLongNotes = sqlite3_column_int(stmt, idx++);
	chartMeta->TotalScratchNotes = sqlite3_column_int(stmt, idx++);
	chartMeta->TotalBackSpinNotes = sqlite3_column_int(stmt, idx++);
	return chartMeta;
}
