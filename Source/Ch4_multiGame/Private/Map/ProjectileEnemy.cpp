#include "Map/ProjectileEnemy.h"
#include "Components/SphereComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Map/EnemyProjectile.h"

AProjectileEnemy::AProjectileEnemy()
{
	DetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphere"));
	DetectionSphere->SetupAttachment(RootComponent);
	DetectionSphere->SetSphereRadius(500.0f);
	
	Gun = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Gun"));
	Gun->SetupAttachment(GetMesh(), TEXT("GunSocket"));
	
	ProjectileSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ProjectileSpawnPoint"));
	ProjectileSpawnPoint->SetupAttachment(Gun);
}

void AProjectileEnemy::BeginPlay()
{
	Super::BeginPlay();

	DetectionSphere->OnComponentBeginOverlap.AddDynamic(
		this,
		&AProjectileEnemy::OnDetectionBeginOverlap
	);
	
	DetectionSphere->OnComponentEndOverlap.AddDynamic(
	this,
	&AProjectileEnemy::OnDetectionEndOverlap
	);
}

void AProjectileEnemy::OnDetectionBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!OtherActor)
	{
		return;
	}

	ACharacter* PlayerCharacter = Cast<ACharacter>(OtherActor);

	if (PlayerCharacter && PlayerCharacter->IsPlayerControlled())
	{
		TargetCharacter = PlayerCharacter;
		bIsAiming = true;

		GetWorld()->GetTimerManager().SetTimer(
		FireTimerHandle,
		this,
		&AProjectileEnemy::StartFiring,
		FirstFireDelay,
		false
		);
		
		UE_LOG(LogTemp, Warning, TEXT("Player entered enemy detection range!"));
	}
}

void AProjectileEnemy::OnDetectionEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if (OtherActor == TargetCharacter)
	{
		TargetCharacter = nullptr;
		bIsAiming = false;

		GetWorld()->GetTimerManager().ClearTimer(FireTimerHandle);
		
		UE_LOG(LogTemp, Warning, TEXT("Player left enemy detection range!"));
	}
}

void AProjectileEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!TargetCharacter)
	{
		return;
	}

	FVector BodyDirection = TargetCharacter->GetActorLocation() - GetActorLocation();
	BodyDirection.Z = 0.0f;

	if (!BodyDirection.IsNearlyZero())
	{
		SetActorRotation(BodyDirection.Rotation());
	}

	FVector ShoulderLocation = GetMesh()->GetBoneLocation(TEXT("Right-shoulder"));
	FVector AimDirection = TargetCharacter->GetActorLocation() - ShoulderLocation;
	FVector LocalAimDirection = GetActorTransform().InverseTransformVectorNoScale(AimDirection);

	float AimPitch = FMath::RadiansToDegrees(
		FMath::Atan2(
			LocalAimDirection.Z,
			FVector2D(LocalAimDirection.X, LocalAimDirection.Y).Size()
		)
	);

	AimRotation = FRotator(AimPitch, 0.0f, 0.0f);

	UE_LOG(LogTemp, Warning, TEXT("Aim Pitch: %f"), AimRotation.Pitch);
}

void AProjectileEnemy::FireProjectile()
{
	if (!ProjectileClass || !ProjectileSpawnPoint)
	{
		return;
	}

	FVector SpawnLocation = ProjectileSpawnPoint->GetComponentLocation();
	FRotator SpawnRotation = ProjectileSpawnPoint->GetComponentRotation();

	GetWorld()->SpawnActor<AEnemyProjectile>(
		ProjectileClass,
		SpawnLocation,
		SpawnRotation
	);
}

void AProjectileEnemy::StartFiring()
{
	if (!TargetCharacter)
	{
		return;
	}
	
	FireProjectile();

	GetWorld()->GetTimerManager().SetTimer(
		FireTimerHandle,
		this,
		&AProjectileEnemy::FireProjectile,
		FireInterval,
		true
	);
}

void AProjectileEnemy::ApplyRecoil()
{
	RecoilPitch = RecoilAmount;

	FTimerHandle RecoilTimerHandle;

	GetWorld()->GetTimerManager().SetTimer(
		RecoilTimerHandle,
		[this]()
		{
			RecoilPitch = 0.0f;
		},
		0.1f,
		false
	);
}
