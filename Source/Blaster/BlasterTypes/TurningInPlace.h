#pragma once


UENUM(BlueprintType)
enum  class ETurningInPlace : uint8
{
	ETIP_Left UMETA(DisplayName = "Turning In Place Left"),
	ETIP_Right UMETA(DisplayName = "Turning In Place Right"),
	ETIP_NotTurning UMETA(DisplayName = "Not Turning In Place"),
	ETIP_MAX UMETA(DisplayName = "MAX")
};
