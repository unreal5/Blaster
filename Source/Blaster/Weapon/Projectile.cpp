

#include "Character/BlasterCharacter.h"
#include "Blaster.h"
#include "Weapon/Projectile.h"

#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

#include "Kismet/GameplayStatics.h"

#include "Sound/SoundCue.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECC_SkeletalMesh, ECR_Block);
	
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->SetUpdatedComponent(GetRootComponent());
	ProjectileMovement->InitialSpeed = 15000.f;
	ProjectileMovement->MaxSpeed = 15000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.f;
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	if (Tracer.Get())
	{
		TracerComponent = UGameplayStatics::SpawnEmitterAttached(Tracer.Get(), RootComponent, NAME_None,
		                                                         /*bone name,但我们不需要挂接到骨骼上，所以传入NAME_None*/
		                                                         GetActorLocation(), GetActorRotation(),
		                                                         EAttachLocation::KeepWorldPosition);
	}
	if (HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
	}
}

void AProjectile::Destroyed()
{
	Super::Destroyed();
	if (ImpactParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticle, GetActorLocation(),
		                                         GetActorRotation());
	}
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
}

void AProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                        FVector NormalImpulse, const FHitResult& Hit)
{
	// 只有Server才注册OhHit事件，因此此函数只会在Server上调用
	auto BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
	{
		BlasterCharacter->Multicast_Hit();
	}
	Destroy();
}
