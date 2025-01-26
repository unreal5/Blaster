// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"

class UCharacterOverlay;
/**
 * HUD for the Blaster game
 */
USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()

	UPROPERTY()
	UTexture2D* CrosshairCenter;
	UPROPERTY()
	UTexture2D* CrosshairLeft;
	UPROPERTY()
	UTexture2D* CrosshairRight;
	UPROPERTY()
	UTexture2D* CrosshairTop;
	UPROPERTY()
	UTexture2D* CrosshairBottom;

	float CrosshairSpread = 1.0f;
	FLinearColor CrosshairColor = FLinearColor::White;
};

UCLASS()
class BLASTER_API ABlasterHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
	virtual void BeginPlay() override;
	
	UPROPERTY(EditDefaultsOnly, Category = "Player Stats")
	TSubclassOf<UCharacterOverlay> CharacterOverlayClass;
	
	UPROPERTY()
	UCharacterOverlay* CharacterOverlay;
private:
	FHUDPackage HUDPackage;
	void DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, const FLinearColor& Color = FLinearColor::White);

	UPROPERTY(EditDefaultsOnly, Category = "Crosshair" , meta = (AllowPrivateAccess = "true"))
	float CrosshairSpreadMax = 16.0f;

	void AddCharacterOverlay();
public:
	FORCEINLINE void SetHudPackage(const FHUDPackage& Package) { HUDPackage = Package; }
	
};
