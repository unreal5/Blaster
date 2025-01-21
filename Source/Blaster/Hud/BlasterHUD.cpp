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
		DrawCrosshair(HUDPackage.CrosshairCenter, ViewportCenter);

		if (HUDPackage.CrosshairLeft == nullptr || HUDPackage.CrosshairRight == nullptr || HUDPackage.CrosshairTop == nullptr || HUDPackage.CrosshairBottom == nullptr) return;

		DrawCrosshair(HUDPackage.CrosshairLeft, ViewportCenter);
		DrawCrosshair(HUDPackage.CrosshairRight, ViewportCenter);
		DrawCrosshair(HUDPackage.CrosshairTop, ViewportCenter);
		DrawCrosshair(HUDPackage.CrosshairBottom, ViewportCenter);
	}
}

void ABlasterHUD::DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter)
{
	if (Texture == nullptr) return;

	const FVector2D TextureSize{static_cast<float>(Texture->GetSizeX()), static_cast<float>(Texture->GetSizeY())};
	const FVector2D CrosshairPosition{ViewportCenter.X - TextureSize.X / 2.0f, ViewportCenter.Y - TextureSize.Y / 2.0f};
	DrawTexture(Texture, CrosshairPosition.X, CrosshairPosition.Y, TextureSize.X, TextureSize.Y, 0.0f, 0.0f, 1.0f, 1.0f, FLinearColor::White, EBlendMode::BLEND_Translucent);
}
