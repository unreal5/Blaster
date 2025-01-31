// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterComponent/CombatComponent.h"

#include "Camera/CameraComponent.h"
#include "Character/BlasterCharacter.h"


#include "Engine/SkeletalMeshSocket.h"

#include "GameFramework/CharacterMovementComponent.h"

#include "Kismet/GameplayStatics.h"

#include "Net/UnrealNetwork.h"

#include "Weapon/Weapon.h"
#include "PlayerController/BlasterPlayerController.h"
#include "Hud/BlasterHUD.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	//IMPORTANT! 必须设置为true，否则组件将不调用InitializeComponent
	bWantsInitializeComponent = true;
}

void UCombatComponent::InitializeComponent()
{
	Super::InitializeComponent();
	Character = Cast<ABlasterCharacter>(GetOwner());
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	//check((void*)GetOwner() == Character.Get());
	if (Character.IsValid())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
		if (Character->GetFollowCamera())
		{
			CurrentFOV = DefaultFOV = Character->GetFollowCamera()->FieldOfView;
		}
	}
}

void UCombatComponent::SetHudCrosshair(float DeltaTime)
{
	if (!Character.IsValid() || Character->Controller == nullptr) return;

	if (!BlasterPlayerController.IsValid())
	{
		BlasterPlayerController = Character->GetController<ABlasterPlayerController>();
		if (BlasterPlayerController.IsValid())
		{
			BlasterHUD = BlasterPlayerController->GetHUD<ABlasterHUD>();
		}
	}
	if (BlasterHUD.IsValid())
	{
		if (EquippedWeapon)
		{
			HUDPackage.CrosshairCenter = EquippedWeapon->CrosshairCenter;
			HUDPackage.CrosshairLeft = EquippedWeapon->CrosshairLeft;
			HUDPackage.CrosshairRight = EquippedWeapon->CrosshairRight;
			HUDPackage.CrosshairTop = EquippedWeapon->CrosshairTop;
			HUDPackage.CrosshairBottom = EquippedWeapon->CrosshairBottom;
		}
		else
		{
			HUDPackage.CrosshairCenter = nullptr;
			HUDPackage.CrosshairLeft = nullptr;
			HUDPackage.CrosshairRight = nullptr;
			HUDPackage.CrosshairTop = nullptr;
			HUDPackage.CrosshairBottom = nullptr;
		}
		// Calculate the spread of the crosshair
		//[0,max] -> [0,1]
		FVector2D WalkSpeedRange{0.f, Character->GetCharacterMovement()->MaxWalkSpeed};
		FVector2D VelocityMultiplierRange{0.f, 1.f};
		FVector Velocity = Character->GetVelocity();
		Velocity.Z = 0;
		CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange,
		                                                            Velocity.Size());

		if (Character->GetCharacterMovement()->IsFalling())
		{
			CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
		}
		else
		{
			CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
		}

		if (bAiming)
		{
			CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.58f, DeltaTime, 30.f);
		}
		else
		{
			CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
		}

		CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 40.f);

		HUDPackage.CrosshairSpread = 0.5f + CrosshairVelocityFactor + CrosshairInAirFactor - CrosshairAimFactor +
			CrosshairShootingFactor;
		BlasterHUD->SetHudPackage(HUDPackage);
	}
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                     FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);


	// 只有本机控制的角色才能进行射击
	if (Character.IsValid() && Character->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrosshair(HitResult);
		HitTarget = HitResult.ImpactPoint;

		SetHudCrosshair(DeltaTime);
		// FOV
		InterpFOV(DeltaTime);
	}
	else
	{
		HitTarget = FVector::ZeroVector;
	}
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (!Character.IsValid()) return;
	if (!WeaponToEquip) return;

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	auto SkeletalMesh = Character->GetMesh();
	if (!SkeletalMesh) return;
	auto RightHandSocket = SkeletalMesh->GetSocketByName("RightHandSocket");
	if (RightHandSocket)
	{
		RightHandSocket->AttachActor(EquippedWeapon, SkeletalMesh);
	}
	EquippedWeapon->SetOwner(Character.Get());
	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, EquippedWeapon);
	DOREPLIFETIME(ThisClass, bAiming);
}

void UCombatComponent::SetAiming(bool bNewAiming)
{
	if (bNewAiming == bAiming) return;
	if (!Character.IsValid()) return;

	// 技巧：客户端立即响应（不需要等待RPC延迟）
	bAiming = bNewAiming;

	//如果位于客户端，则需要别的客户端也同步显示。
	// 进一步思考，可以略过if判断（位于服务端时，会两次设置bAiming为同一值）
	//if (!Character->HasAuthority())
	Server_SetAiming(bNewAiming);

	Character->GetCharacterMovement()->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;
}


void UCombatComponent::Server_SetAiming_Implementation(bool bNewAiming)
{
	check(Character.IsValid() && Character->HasAuthority());

	bAiming = bNewAiming;
	Character->GetCharacterMovement()->MaxWalkSpeed = bAiming ? AimWalkSpeed : BaseWalkSpeed;
}

void UCombatComponent::OnRep_EquippedWeapon(AWeapon* OldWeapon)
{
	if (EquippedWeapon && Character.IsValid())
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

		auto SkeletalMesh = Character->GetMesh();
		if (!SkeletalMesh) return;
		auto RightHandSocket = SkeletalMesh->GetSocketByName("RightHandSocket");
		if (RightHandSocket)
		{
			RightHandSocket->AttachActor(EquippedWeapon, SkeletalMesh);
		}
		
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}
}

void UCombatComponent::TraceUnderCrosshair(FHitResult& TraceHitResult)
{
	TraceHitResult.bBlockingHit = false;

	FVector2D ViewportSize;
	if (!Character.IsValid()) return;
	APlayerController* PC = Character->GetController<APlayerController>();
	if (!PC) return;
	if (!PC->IsLocalController())
	{
		UE_LOG(LogTemp, Warning, TEXT("Not Local Controller"));
		return;
	}
	if (!GEngine || !GEngine->GameViewport) return;


	GEngine->GameViewport->GetViewportSize(ViewportSize);
	FVector2D CrosshairLocation(ViewportSize.X / 2, ViewportSize.Y / 2);
	FVector CrosshairWorldPosition, CrosshairWorldDirection;
	// 对于客户端UGameplayStatics::GetPlayerController(this,0);返回的总是代表玩家的控制器，但对于服务端则不成立。
	//UGameplayStatics::GetPlayerController(this,0);
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(PC, CrosshairLocation, CrosshairWorldPosition,
	                                                               CrosshairWorldDirection);
	if (bScreenToWorld)
	{
		FVector TraceStart = CrosshairWorldPosition;
		// 起始点前移，防止碰撞到后面的目标
		const float DistanceToCharacter = (Character->GetActorLocation() - TraceStart).Size();
		TraceStart += CrosshairWorldDirection * DistanceToCharacter;

		FVector TraceEnd = CrosshairWorldPosition + CrosshairWorldDirection * TRACE_LENGTH;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(Character.Get());
		QueryParams.AddIgnoredActor(EquippedWeapon);
		QueryParams.bTraceComplex = true;
		QueryParams.bReturnPhysicalMaterial = true;
		GetWorld()->LineTraceSingleByChannel(TraceHitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
		// 如果没有碰到物体，例如看向天空，则TraceHitResult也应设置有效的值，我们可以向天空开枪。
		if (!TraceHitResult.bBlockingHit)
		{
			TraceHitResult.ImpactPoint = TraceEnd;
		}
		if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshair>())
		{
			// tell hud draw crosshair to red color.
			HUDPackage.CrosshairColor = FLinearColor::Red;
		}
		else
		{
			HUDPackage.CrosshairColor = FLinearColor::White;
		}
	}
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
	if (EquippedWeapon == nullptr) return;

	if (bAiming)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime,
		                              EquippedWeapon->GetZoomInterpSpeed());
	}
	else
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	if (Character.IsValid() && Character->GetFollowCamera())
	{
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
	}
}

void UCombatComponent::StartFireTimer()
{
	if (EquippedWeapon == nullptr || !Character.IsValid()) return;

	Character->GetWorldTimerManager().SetTimer(FireTimer, this, &UCombatComponent::FireTimerFinished,
	                                           EquippedWeapon->FireDelay);
}

void UCombatComponent::FireTimerFinished()
{
	bCanFire = true;

	if (EquippedWeapon == nullptr) return;

	if (bFireButtonPressed && EquippedWeapon->bAutmatic)
	{
		Fire();
	}
}


void UCombatComponent::Fire()
{
	if (!bCanFire) return;

	bCanFire = false;
	Server_Fire(HitTarget);

	if (EquippedWeapon)
	{
		CrosshairShootingFactor += 0.75f;

		if (EquippedWeapon->bAutmatic)
		{
			StartFireTimer();
		}
	}
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;
	if (bFireButtonPressed)
	{
		Fire();
	}
}


void UCombatComponent::Server_Fire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	Multicast_Fire(TraceHitTarget);
}

void UCombatComponent::Multicast_Fire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (EquippedWeapon == nullptr) return;

	if (Character.IsValid())
	{
		Character->PlayFireMontage(bAiming);

		EquippedWeapon->Fire(TraceHitTarget);
	}
}
