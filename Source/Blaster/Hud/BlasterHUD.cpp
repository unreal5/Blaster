// Fill out your copyright notice in the Description page of Project Settings.


#include "Hud/BlasterHUD.h"

void ABlasterHUD::DrawHUD()
{
	Super::DrawHUD();

	FVector2D ViewportSize;
	if (GEngine)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
		const FVector2D ViewportCenter{ViewportSize.X / 2.0f, ViewportSize.Y / 2.0f};
		DrawCrosshair(HUDPackage.CrosshairCenter, ViewportCenter, FVector2D::ZeroVector, HUDPackage.CrosshairColor);

		if (HUDPackage.CrosshairLeft == nullptr || HUDPackage.CrosshairRight == nullptr || HUDPackage.CrosshairTop ==
			nullptr || HUDPackage.CrosshairBottom == nullptr) return;

		float SpreadScaled = CrosshairSpreadMax * HUDPackage.CrosshairSpread;
		DrawCrosshair(HUDPackage.CrosshairLeft, ViewportCenter, FVector2D{-SpreadScaled, 0.0f}, HUDPackage.CrosshairColor);
		DrawCrosshair(HUDPackage.CrosshairRight, ViewportCenter, FVector2D{SpreadScaled, 0.0f}, HUDPackage.CrosshairColor);
		DrawCrosshair(HUDPackage.CrosshairTop, ViewportCenter, FVector2D{0.0f, -SpreadScaled}, HUDPackage.CrosshairColor);
		DrawCrosshair(HUDPackage.CrosshairBottom, ViewportCenter, FVector2D{0.0f, SpreadScaled}, HUDPackage.CrosshairColor);
	}
}

void ABlasterHUD::DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, const FLinearColor& Color)
{
	if (Texture == nullptr) return;

	const FVector2D TextureSize{static_cast<float>(Texture->GetSizeX()), static_cast<float>(Texture->GetSizeY())};
	const FVector2D CrosshairPosition{
		ViewportCenter.X - TextureSize.X / 2.0f + Spread.X, ViewportCenter.Y - TextureSize.Y / 2.0f + Spread.Y
	};
	DrawTexture(Texture, CrosshairPosition.X, CrosshairPosition.Y, TextureSize.X, TextureSize.Y, 0.0f, 0.0f, 1.0f, 1.0f,
	            Color, EBlendMode::BLEND_Translucent);
}
