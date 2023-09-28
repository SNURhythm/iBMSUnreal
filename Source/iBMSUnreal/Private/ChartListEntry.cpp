// Fill out your copyright notice in the Description page of Project Settings.


#include "ChartListEntry.h"

#include "ChartListEntryData.h"

void UChartListEntry::OnButtonClicked()
{
	if (OnEntryClicked.IsBound())
	{
		OnEntryClicked.Execute(EntryData);
	}
}

void UChartListEntry::NativeConstruct()
{
	Super::NativeConstruct();
	//Button->OnClicked.AddDynamic(this, &UChartListEntry::OnButtonClicked);
}

void UChartListEntry::NativeOnListItemObjectSet(UObject* InObject)
{
	EntryData = Cast<UChartMeta>(InObject);
	if (EntryData)
	{
		TitleText->SetText(FText::FromString(EntryData->Title));
		ArtistText->SetText(FText::FromString(EntryData->Artist));
		KeyModeText->SetText(FText::FromString(EntryData->KeyMode+"k"));
	}
}
