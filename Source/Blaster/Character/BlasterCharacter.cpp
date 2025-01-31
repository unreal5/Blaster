#include "Character/BlasterCharacter.h"
#include "Blaster.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

#include "BlasterComponent/CombatComponent.h"

#include "Camera/CameraComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameMode/BlasterGameMode.h"

#include "Kismet/KismetMathLibrary.h"

#include "Net/UnrealNetwork.h"
#include "PlayerController/BlasterPlayerController.h"

#include "Weapon/Weapon.h"

ABlasterCharacter::ABlasterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 600.0f;
	CameraBoom->SocketOffset = FVector(0.f, 55.f, 65.f);
	CameraBoom->bUsePawnControlRotation = true;
	//CameraBoom->bDoCollisionTest = false;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);

	OverheadWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidgetComponent"));
	OverheadWidgetComponent->SetupAttachment(RootComponent);

	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	//组件类处理复制只需要以下代码，然后组件自身再按照复制的要求（声明复制变量，注册复制变量……）处理
	// 组件自身不需要在GetLifetimeReplicatedProps中注册，因为组件类的GetLifetimeReplicatedProps会被调用
	CombatComponent->SetIsReplicated(true);

	// 调整下蹲参数
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
	GetCharacterMovement()->MaxWalkSpeedCrouched = 100.f;
	GetCharacterMovement()->SetCrouchedHalfHeight(60.f);

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// 设置网络更新频率
	SetNetUpdateFrequency(66.f);
	SetMinNetUpdateFrequency(33.f);

	// 生成策略
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// timeline componet
	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("溶解时间线组件"));
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	// 在Combat组件的BeginPlay中，也可以设置
	//if (CombatComponent)
	//{
	//	CombatComponent->Character = this;
	//}
}

void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();
	OverheadWidgetComponent->SetVisibility(true);

	// BlasterPlayerController = GetController<ABlasterPlayerController>();
	// if (BlasterPlayerController)
	// {
	// 	BlasterPlayerController->SetHUDHealth(Health, MaxHealth);
	// }
	UpdateHudHealth();

	// 只在服务器上注册接收伤害事件
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ThisClass::ReceiveDamage);
	}
}

void ABlasterCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Add Input Mapping Context
	BlasterPlayerController = GetController<ABlasterPlayerController>();
	if (!BlasterPlayerController) return;
	ULocalPlayer* LocalPlayer = BlasterPlayerController->GetLocalPlayer();
	if (!LocalPlayer) return;
	UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!Subsystem) return;

	Subsystem->AddMappingContext(DefaultMappingContext, 0);
}

void ABlasterCharacter::Jump()
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Super::Jump();
	}
}

void ABlasterCharacter::ElimTimerFinished()
{
	GetWorld()->GetAuthGameMode<ABlasterGameMode>()->RequestRespawn(this, GetController());
}

// only called on server
void ABlasterCharacter::Elim()
{
	if (CombatComponent->GetEquippedWeapon())
    {
        CombatComponent->GetEquippedWeapon()->Dropped();
    }
	MulticastElim();
	GetWorldTimerManager().SetTimer(ElimTimerHandle, this, &ThisClass::ElimTimerFinished, ElimDelay, false);
}

void ABlasterCharacter::MulticastElim_Implementation()
{
	bElimmed = true;
	PlayElimMontage();

	// dissolve effect
	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	StartDissolve();
	// disable character movement
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	if (BlasterPlayerController)
	{
		DisableInput(BlasterPlayerController);
	}
	// disable collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ThisClass, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ThisClass, Health);
}

void ABlasterCharacter::OnRep_ReplicateMovement()
{
	Super::OnRep_ReplicateMovement();

	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.f;
}

void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* OldWeapon)
{
	if (OldWeapon) OldWeapon->ShowPickupWidget(false);
	if (OverlappingWeapon) OverlappingWeapon->ShowPickupWidget(true);
}


void ABlasterCharacter::EquipButtonPressed()
{
	check(CombatComponent);

	// 请求服务器执行EquipWeapon
	if (!OverlappingWeapon) return;

	if (HasAuthority())
	{
		CombatComponent->EquipWeapon(OverlappingWeapon);
	}
	else
	{
		Server_EquipButtonPressed();
	}
}

void ABlasterCharacter::CrouchButtonPressed()
{
	bIsCrouched ? UnCrouch() : Crouch();
}

void ABlasterCharacter::AimButtonPressed()
{
	CombatComponent->SetAiming(true);
}

void ABlasterCharacter::AimButtonReleased()
{
	CombatComponent->SetAiming(false);
}

// void ABlasterCharacter::FireButtonPressed()
// {
// 	CombatComponent->FireButtonPressed(true);
// }
//
// void ABlasterCharacter::FireButtonReleased()
// {
// 	CombatComponent->FireButtonPressed(false);
// }

void ABlasterCharacter::Server_EquipButtonPressed_Implementation()
{
	if (!OverlappingWeapon) return;
	CombatComponent->EquipWeapon(OverlappingWeapon);
}

/*
void ABlasterCharacter::Multicast_Hit_Implementation()
{
	PlayHitReactMontage();
}
*/

void ABlasterCharacter::HideCameraIfCharacterClose()
{
	if (!IsLocallyControlled()) return;

	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		GetMesh()->SetVisibility(false);
		if (CombatComponent->GetEquippedWeapon())
		{
			CombatComponent->GetEquippedWeapon()->GetWeaponMesh()->bOwnerNoSee = true;
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if (CombatComponent->GetEquippedWeapon())
		{
			CombatComponent->GetEquippedWeapon()->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}

void ABlasterCharacter::UpdateHudHealth()
{
	if (BlasterPlayerController && BlasterPlayerController->IsLocalController())
	{
		BlasterPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void ABlasterCharacter::OnRep_Health()
{
	PlayHitReactMontage();
	UpdateHudHealth();
}


void ABlasterCharacter::UpdateDissolveMaterial(float Value)
{
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), Value);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f - 200.f * Value);
	}
}

void ABlasterCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &ThisClass::UpdateDissolveMaterial);
	if (DissolveCurve)
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->PlayFromStart();
	}
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	if (OverlappingWeapon != Weapon)
	{
		AWeapon* OldWeapon = OverlappingWeapon;
		OverlappingWeapon = Weapon;
		// 我们目前位于服务器，对于服务器，不会调用OnRep_OverlappingWeapon
		// 我们需要知道当前是否是本机控制的角色，如果是，我们需要手动调用OnRep_OverlappingWeapon的逻辑
		if (IsLocallyControlled())
		{
			OnRep_OverlappingWeapon(OldWeapon);
		}
	}
}


void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 每帧更新瞄准偏移
	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	else
	{
		TimeSinceLastMovementReplication += DeltaTime;
		if (TimeSinceLastMovementReplication > 0.25f)
		{
			OnRep_ReplicateMovement();
		}

		CalculateAO_Pitch();
	}
	// 用RepNOtify来处理
	// else
	// {
	// 	SimProxiesTurn();
	// }

	HideCameraIfCharacterClose();
}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	auto EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	auto MoveActionLambda = [this](const FInputActionValue& Value)
	{
		// input is a Vector2D
		FVector2D InputVector = Value.Get<FVector2D>();
		if (Controller != nullptr)
		{
			const FRotator YawRotation(0, Controller->GetControlRotation().Yaw, 0);
			const FVector MovementDirection{InputVector.Y, InputVector.X, 0.f};
			auto MovementVector = YawRotation.RotateVector(MovementDirection);
			MovementVector.Z = 0.f;
			AddMovementInput(MovementVector.GetSafeNormal());
		}
	};
	EnhancedInputComponent->BindActionValueLambda(MoveAction, ETriggerEvent::Triggered, MoveActionLambda);

	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ThisClass::Jump);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ThisClass::StopJumping);

	auto LookActionLambda = [this](const FInputActionValue& Value)
	{
		// input is a Vector2D
		FVector2D LookAxisVector = Value.Get<FVector2D>();

		if (Controller != nullptr)
		{
			// add yaw and pitch input to controller
			AddControllerYawInput(LookAxisVector.X);
			AddControllerPitchInput(LookAxisVector.Y);
		}
	};
	EnhancedInputComponent->BindActionValueLambda(LookAction, ETriggerEvent::Triggered, LookActionLambda);

	// 装备武器
	EnhancedInputComponent->BindAction(EquipAction, ETriggerEvent::Started, this,
	                                   &ThisClass::EquipButtonPressed);

	// 下蹲
	EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this,
	                                   &ThisClass::CrouchButtonPressed);

	// 瞄准
	EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this,
	                                   &ThisClass::AimButtonPressed);
	EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this,
	                                   &ThisClass::AimButtonReleased);

	// 开火
	EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, CombatComponent,
	                                   &UCombatComponent::FireButtonPressed, true);
	EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, CombatComponent,
	                                   &UCombatComponent::FireButtonPressed, false);
}

bool ABlasterCharacter::IsWeaponEquipped() const
{
	return (CombatComponent && CombatComponent->GetEquippedWeapon() != nullptr);
}

bool ABlasterCharacter::IsAiming() const
{
	return CombatComponent && CombatComponent->IsAiming();
}

AWeapon* ABlasterCharacter::GetEquippedWeapon() const
{
	return CombatComponent ? CombatComponent->GetEquippedWeapon() : nullptr;
}

void ABlasterCharacter::PlayFireMontage(bool bAiming)
{
	if (CombatComponent->GetEquippedWeapon() == nullptr) return;


	// UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	// if (AnimInstance == nullptr || FireWeaponMontage == nullptr) return;
	//
	// AnimInstance->Montage_Play(FireWeaponMontage);
	FName SectionName = bAiming ? TEXT("RifleAim") : TEXT("RifleHip");
	// AnimInstance->Montage_JumpToSection(SectionName, FireWeaponMontage);
	PlayAnimMontage(FireWeaponMontage, 1.f, SectionName);
}

void ABlasterCharacter::PlayHitReactMontage()
{
	check(HitReactMontage);
	// 蒙太奇建立在持枪的基础上！！！
	if (CombatComponent->GetEquippedWeapon() == nullptr) return;
	FName SectionName = TEXT("FromFront");
	PlayAnimMontage(HitReactMontage, 1.f, SectionName);
}

void ABlasterCharacter::PlayElimMontage()
{
	check(ElimMontage);
	PlayAnimMontage(ElimMontage);
}

FVector ABlasterCharacter::GetHitTarget() const
{
	return CombatComponent ? CombatComponent->GetHitTarget() : FVector::ZeroVector;
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
	//UE_LOG(LogTemp, Warning, TEXT("AO_Yaw: %f"), AO_Yaw);
	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}

	// 正在转身
	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 10.f);
		AO_Yaw = InterpAO_Yaw;

		if (FMath::Abs(AO_Yaw) < 15.f) // 转身结束
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			// 记录起始瞄准角度
			StartingAimRotator = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

void ABlasterCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.f)
	{
		// fix the pitch value when looking up
		FVector2D InRange{270.f, 360.f};
		FVector2D OutRange{-90.f, 0.f};
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

float ABlasterCharacter::CalculateSpeed()
{
	auto Velocity = GetVelocity();
	Velocity.Z = 0;
	auto Speed = Velocity.Size();
	return Speed;
}

void ABlasterCharacter::AimOffset(float DeltaTime)
{
	if (!CombatComponent->GetEquippedWeapon())
		return;

	float Speed = CalculateSpeed();
	bool bIsInAir = GetCharacterMovement()->IsFalling();
	if (FMath::IsNearlyZero(Speed) && !bIsInAir) // standing still, not jumping
	{
		bRotateRootBone = true;
		FRotator CurrentAimRotation = FRotator{0.f, GetBaseAimRotation().Yaw, 0.f};
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotator);
		AO_Yaw = DeltaAimRotation.Yaw;
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}
		TurnInPlace(DeltaTime);
	}
	else // in air or moving
	{
		bRotateRootBone = false;
		StartingAimRotator = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	CalculateAO_Pitch();
}

void ABlasterCharacter::SimProxiesTurn()
{
	if (CombatComponent->GetEquippedWeapon() == nullptr) return;
	bRotateRootBone = false;
	if (CalculateSpeed() > 0.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}


	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;
	if (FMath::Abs(ProxyYaw) > TurnThreshold) // 教程逻辑有问题
	{
		if (ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else //if (ProxyYaw < -TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		// else
		// {
		// 	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		// }
	}
	else
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}
}

void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	// 服务端不会调用OnRep_Health，因此没有命中效果，我们需要手动调用
	UpdateHudHealth();
	PlayHitReactMontage();
	if (Health <= 0.f)
	{
		// 通知GameMode
		auto BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
		check(BlasterGameMode);
		if (BlasterGameMode)
		{
			check(BlasterPlayerController);
			BlasterGameMode->PlayerEliminated(this, BlasterPlayerController, Cast<ABlasterPlayerController>(InstigatedBy));
		}
	}
}
