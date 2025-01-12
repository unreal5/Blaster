// Fill out your copyright notice in the Description page of Project Settings.


#include "Hud/OverheadWidget.h"

#include "Components/TextBlock.h"

namespace
{
	FString GetEnumDisplayName(ENetRole Role)
	{
		const UEnum* EnumPtr = FindObject<UEnum>(ANY_PACKAGE, TEXT("ENetRole"), true);
		if (!EnumPtr)
		{
			return FString("Invalid");
		}

		return EnumPtr->GetDisplayNameTextByValue(static_cast<int64>(Role)).ToString();
	}
}


void UOverheadWidget::SetDisplayText(FString TextToDisplay)
{
	if (!DisplayText) return;

	DisplayText->SetText(FText::FromString(TextToDisplay));
	
}

void UOverheadWidget::ShowPlayerNetRole(APawn* InPawn)
{
	if (!DisplayText) return;

	auto LocalRole = InPawn->GetLocalRole();
	auto RemoteRole = InPawn->GetRemoteRole();
	auto LocalString = GetEnumDisplayName(LocalRole);
	auto RemoteString = GetEnumDisplayName(RemoteRole);
	auto TextToDisplay = FString::Printf(TEXT("Local: %s\nRemote: %s"), *LocalString, *RemoteString);
	SetDisplayText(TextToDisplay);
}

void UOverheadWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (DisplayText)
	{
		DisplayText->SynchronizeProperties();
	}
}
