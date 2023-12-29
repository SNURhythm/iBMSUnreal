// Fill out your copyright notice in the Description page of Project Settings.

#include "ChartDBHelper.h"
#include "iOSNatives.h"
#include "iBMSUnreal/Public/Utils.h"


sqlite3* ChartDBHelper::Connect() {
	IFileManager& FileManager = IFileManager::Get();
	FString Directory = FUtils::GetDocumentsPath("db");
	FileManager.MakeDirectory(*Directory, true);
	FString pathRel = FPaths::Combine(Directory, "chart.db");
	FString path = IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*pathRel);
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
bool ChartDBHelper::CreateChartMetaTable(sqlite3* db) {
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
		FString err = UTF8_TO_TCHAR(errMsg);
		UE_LOG(LogTemp, Error, TEXT("SQL error while creating chart meta table: %s"), *err);
		sqlite3_free(errMsg);
		return false;
	}
	return true;
}

bool ChartDBHelper::InsertChartMeta(sqlite3* db, FChartMeta& chartMeta) {
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
		FString err = UTF8_TO_TCHAR(sqlite3_errmsg(db));
		UE_LOG(LogTemp, Error, TEXT("SQL error while preparing statement to insert a chart: %s"), *err);
		sqlite3_close(db);
		return false;
	}
	sqlite3_bind_text(stmt, 1, TCHAR_TO_UTF8(*ToRelativePath(chartMeta.BmsPath)), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 2, TCHAR_TO_UTF8(*chartMeta.MD5), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 3, TCHAR_TO_UTF8(*chartMeta.SHA256), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 4, TCHAR_TO_UTF8(*chartMeta.Title), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 5, TCHAR_TO_UTF8(*chartMeta.SubTitle), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 6, TCHAR_TO_UTF8(*chartMeta.Genre), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 7, TCHAR_TO_UTF8(*chartMeta.Artist), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 8, TCHAR_TO_UTF8(*chartMeta.SubArtist), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(stmt, 9, TCHAR_TO_UTF8(*ToRelativePath(chartMeta.Folder)), -1, SQLITE_TRANSIENT);
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
		FString err = UTF8_TO_TCHAR(sqlite3_errmsg(db));
		UE_LOG(LogTemp, Error, TEXT("SQL error while inserting a chart: %s"), *err);
		sqlite3_free(stmt);
		return false;
	}
	sqlite3_finalize(stmt);
	return true;
}

TArray<FChartMeta*> ChartDBHelper::SelectAllChartMeta(sqlite3* db) {
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
		FString err = UTF8_TO_TCHAR(sqlite3_errmsg(db));
		UE_LOG(LogTemp, Error, TEXT("SQL error while getting all charts: %s"), *err);
		sqlite3_free(stmt);
		return TArray<FChartMeta*>();
	}
	TArray<FChartMeta*> chartMetas;
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		auto chartMeta = ReadChartMeta(stmt);
		chartMetas.Add(chartMeta);
	}
	sqlite3_finalize(stmt);
	return chartMetas;
}

TArray<FChartMeta*> ChartDBHelper::SearchChartMeta(sqlite3* db, FString& text) {
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
		FString err = UTF8_TO_TCHAR(sqlite3_errmsg(db));
		UE_LOG(LogTemp, Error, TEXT("SQL error while searching for charts: %s"), *err);
		sqlite3_free(stmt);
		return TArray<FChartMeta*>();
	}
	sqlite3_bind_text(stmt, 1, TCHAR_TO_UTF8(*("%" + text + "%")), -1, SQLITE_TRANSIENT);

	TArray<FChartMeta*> chartMetas;
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		auto chartMeta = ReadChartMeta(stmt);
		chartMetas.Add(chartMeta);
	}
	sqlite3_finalize(stmt);
	return chartMetas;
}

bool ChartDBHelper::DeleteChartMeta(sqlite3* db, FString& path) {
	FString pathRel = ToRelativePath(path);
	auto query = "DELETE FROM chart_meta WHERE path = @path";
	sqlite3_stmt* stmt;
	int rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
	if (rc != SQLITE_OK) {
		FString err = UTF8_TO_TCHAR(sqlite3_errmsg(db));
		UE_LOG(LogTemp, Error, TEXT("SQL error while preparing statement to delete a chart: %s"), *err);
		sqlite3_free(stmt);
		return false;
	}
	sqlite3_bind_text(stmt, 1, TCHAR_TO_UTF8(*pathRel), -1, SQLITE_TRANSIENT);
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		FString err = UTF8_TO_TCHAR(sqlite3_errmsg(db));
		UE_LOG(LogTemp, Error, TEXT("SQL error while deleting a chart: %s"), *err);
		sqlite3_close(db);
		return false;
	}
	sqlite3_finalize(stmt);
	return true;
}

bool ChartDBHelper::ClearChartMeta(sqlite3* db) {
	auto query = "DELETE FROM chart_meta";
	sqlite3_stmt* stmt;
	int rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
	if (rc != SQLITE_OK) {
		FString err = UTF8_TO_TCHAR(sqlite3_errmsg(db));
		UE_LOG(LogTemp, Error, TEXT("SQL error while clearing: %s"), *err);
		sqlite3_free(stmt);
		return false;
	}
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		FString err = UTF8_TO_TCHAR(sqlite3_errmsg(db));
		UE_LOG(LogTemp, Error, TEXT("SQL error while clearing: %s"), *err);
		sqlite3_free(stmt);
		return false;
	}
	sqlite3_finalize(stmt);
	return true;
}

FChartMeta* ChartDBHelper::ReadChartMeta(sqlite3_stmt* stmt) {
	int idx = 0;
	FChartMeta* chartMeta = new FChartMeta();
	FString path = UTF8_TO_TCHAR(sqlite3_column_text(stmt, idx++));
	chartMeta->BmsPath = ToAbsolutePath(path);
	chartMeta->MD5 = UTF8_TO_TCHAR(sqlite3_column_text(stmt, idx++));
	chartMeta->SHA256 = UTF8_TO_TCHAR(sqlite3_column_text(stmt, idx++));
	chartMeta->Title = UTF8_TO_TCHAR(sqlite3_column_text(stmt, idx++));
	chartMeta->SubTitle = UTF8_TO_TCHAR(sqlite3_column_text(stmt, idx++));
	chartMeta->Genre = UTF8_TO_TCHAR(sqlite3_column_text(stmt, idx++));
	chartMeta->Artist = UTF8_TO_TCHAR(sqlite3_column_text(stmt, idx++));
	chartMeta->SubArtist = UTF8_TO_TCHAR(sqlite3_column_text(stmt, idx++));
	FString folder = UTF8_TO_TCHAR(sqlite3_column_text(stmt, idx++));
	chartMeta->Folder = ToAbsolutePath(folder);
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

bool ChartDBHelper::CreateEntriesTable(sqlite3* db)
{
	// save paths to search for charts
	auto query = "CREATE TABLE IF NOT EXISTS entries ("
		"path       TEXT primary key"
		")";

	char* errMsg;
	int rc = sqlite3_exec(db, query, nullptr, nullptr, &errMsg);
	if (rc != SQLITE_OK) {
		FString err = UTF8_TO_TCHAR(sqlite3_errmsg(db));
		UE_LOG(LogTemp, Error, TEXT("SQL error while creating entries table: %s"), *err);
		sqlite3_free(errMsg);
		return false;
	}
	return true;
}

bool ChartDBHelper::InsertEntry(sqlite3* db, FString& path)
{
	auto query = "REPLACE INTO entries ("
		"path"
		") VALUES("
		"@path"
		")";
	sqlite3_stmt* stmt;
	int rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
	if (rc != SQLITE_OK) {
		FString err = UTF8_TO_TCHAR(sqlite3_errmsg(db));
		UE_LOG(LogTemp, Error, TEXT("SQL error while preparing statement to insert an entry: %s"), *err);
		sqlite3_close(db);
		return false;
	}
	sqlite3_bind_text(stmt, 1, TCHAR_TO_UTF8(*path), -1, SQLITE_TRANSIENT);
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		FString err = UTF8_TO_TCHAR(sqlite3_errmsg(db));
		UE_LOG(LogTemp, Error, TEXT("SQL error while inserting an entry: %s"), *err);
		sqlite3_free(stmt);
		return false;
	}
	sqlite3_finalize(stmt);
	return true;
}

TArray<FString> ChartDBHelper::SelectAllEntries(sqlite3* db)
{
	auto query = "SELECT "
		"path"
		" FROM entries";
	sqlite3_stmt* stmt;
	int rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
	if (rc != SQLITE_OK) {
		FString err = UTF8_TO_TCHAR(sqlite3_errmsg(db));
		UE_LOG(LogTemp, Error, TEXT("SQL error while getting all entries: %s"), *err);
		sqlite3_free(stmt);
		return TArray<FString>();
	}
	TArray<FString> entries;
	while (sqlite3_step(stmt) == SQLITE_ROW)
	{
		FString entry = UTF8_TO_TCHAR(sqlite3_column_text(stmt, 0));
		entries.Add(entry);
	}
	sqlite3_finalize(stmt);
	return entries;
}

bool ChartDBHelper::DeleteEntry(sqlite3* db, FString& path)
{
	auto query = "DELETE FROM entries WHERE path = @path";
	sqlite3_stmt* stmt;
	int rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
	if (rc != SQLITE_OK) {
		FString err = UTF8_TO_TCHAR(sqlite3_errmsg(db));
		UE_LOG(LogTemp, Error, TEXT("SQL error while preparing statement to delete an entry: %s"), *err);
		sqlite3_free(stmt);
		return false;
	}
	sqlite3_bind_text(stmt, 1, TCHAR_TO_UTF8(*path), -1, SQLITE_TRANSIENT);
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		FString err = UTF8_TO_TCHAR(sqlite3_errmsg(db));
		UE_LOG(LogTemp, Error, TEXT("SQL error while deleting an entry: %s"), *err);
		sqlite3_close(db);
		return false;
	}
	sqlite3_finalize(stmt);
	return true;
}

bool ChartDBHelper::ClearEntries(sqlite3* db)
{
	auto query = "DELETE FROM entries";
	sqlite3_stmt* stmt;
	int rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
	if (rc != SQLITE_OK) {
		FString err = UTF8_TO_TCHAR(sqlite3_errmsg(db));
		UE_LOG(LogTemp, Error, TEXT("SQL error while clearing: %s"), *err);
		sqlite3_free(stmt);
		return false;
	}
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		FString err = UTF8_TO_TCHAR(sqlite3_errmsg(db));
		UE_LOG(LogTemp, Error, TEXT("SQL error while clearing: %s"), *err);
		sqlite3_free(stmt);
		return false;
	}
	sqlite3_finalize(stmt);
	return true;
}

FString ChartDBHelper::ToRelativePath(FString& path)
{
	// for iOS, remove Documents
#if PLATFORM_IOS
	FString Documents = FPaths::Combine(GetIOSDocumentsPath(), "BMS/");
	FString DocumentsAbs = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*Documents);
	UE_LOG(LogTemp, Log, TEXT("ToRel - DocumentsAbs: %s, Path: %s"), *DocumentsAbs, *path);
	if (path.StartsWith(DocumentsAbs)) {
		UE_LOG(LogTemp, Log, TEXT("Relative Path: %s"), *path.RightChop(DocumentsAbs.Len()));
		return path.RightChop(DocumentsAbs.Len());
	}
#endif
	// otherwise, noop
	return path;
}

FString ChartDBHelper::ToAbsolutePath(FString& path)
{
	// for iOS, add Documents
#if PLATFORM_IOS
	FString Documents = FPaths::Combine(GetIOSDocumentsPath(), "BMS/");
	FString DocumentsAbs = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*Documents);
	UE_LOG(LogTemp, Log, TEXT("ToAbs - DocumentsAbs: %s, Path: %s"), *DocumentsAbs, *path);
	if (!path.StartsWith(DocumentsAbs)) {
		UE_LOG(LogTemp, Log, TEXT("Absolute Path: %s"), *FPaths::Combine(DocumentsAbs, path));
		return FPaths::Combine(DocumentsAbs, path);
	}
#endif
	// otherwise, noop
	return path;
}
