#include "Character/BlasterCharacter.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

#include "BlasterComponent/CombatComponent.h"

#include "Camera/CameraComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

#include "Net/UnrealNetwork.h"

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

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

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

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
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
}

void ABlasterCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Add Input Mapping Context
	APlayerController* PlayerController = GetController<APlayerController>();
	if (!PlayerController) return;
	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	if (!LocalPlayer) return;
	UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!Subsystem) return;

	Subsystem->AddMappingContext(DefaultMappingContext, 0);
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ThisClass, OverlappingWeapon, COND_OwnerOnly);
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

void ABlasterCharacter::Server_EquipButtonPressed_Implementation()
{
	if (!OverlappingWeapon) return;
	CombatComponent->EquipWeapon(OverlappingWeapon);
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
	AimOffset(DeltaTime);
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

	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

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
	                                   &ABlasterCharacter::EquipButtonPressed);

	// 下蹲
	EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this,
	                                   &ABlasterCharacter::CrouchButtonPressed);

	// 瞄准
	EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this,
	                                   &ABlasterCharacter::AimButtonPressed);
	EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this,
	                                   &ABlasterCharacter::AimButtonReleased);
}

bool ABlasterCharacter::IsWeaponEquipped() const
{
	return (CombatComponent && CombatComponent->GetEquippedWeapon() != nullptr);
}

bool ABlasterCharacter::IsAiming() const
{
	return CombatComponent && CombatComponent->IsAiming();
}
void ABlasterCharacter::AimOffset(float DeltaTime)
{
}