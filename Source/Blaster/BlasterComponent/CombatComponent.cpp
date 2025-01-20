// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterComponent/CombatComponent.h"

#include "Character/BlasterCharacter.h"

#include "Components/SphereComponent.h"

#include "Engine/SkeletalMeshSocket.h"

#include "GameFramework/CharacterMovementComponent.h"

#include "Kismet/GameplayStatics.h"

#include "Net/UnrealNetwork.h"

#include "Weapon/Weapon.h"


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
	}
}


void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                     FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FHitResult HitResult;
	TraceUnderCrosshair(HitResult);
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
		else
		{
			DrawDebugSphere(GetWorld(), TraceHitResult.ImpactPoint, 10.0f, 12, FColor::Red, false, 0.0f);
		}
	}
}


void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;
	if (bFireButtonPressed)
	{
		Server_Fire();
	}
}

void UCombatComponent::Server_Fire_Implementation()
{
	Multicast_Fire();
}

void UCombatComponent::Multicast_Fire_Implementation()
{
	if (EquippedWeapon == nullptr) return;

	if (Character.IsValid())
	{
		Character->PlayFireMontage(bAiming);
		EquippedWeapon->Fire();
	}
}
