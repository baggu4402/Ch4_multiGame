#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ProjectileEnemy.generated.h"

class AEnemyProjectile;
class USphereComponent;
class USceneComponent;

UCLASS()
class CH4_MULTIGAME_API AProjectileEnemy : public ACharacter
{
	GENERATED_BODY()

public:
	AProjectileEnemy();
	
	void FireProjectile();
	
	void StartFiring();
	
	void ApplyRecoil();

protected:
	virtual void BeginPlay() override;

	//플레이어 감지 범위
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Detection")
	TObjectPtr<USphereComponent> DetectionSphere;
	
	// 총
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<UStaticMeshComponent> Gun;
	
	//현재 조준 대상
	UPROPERTY()
	TObjectPtr<ACharacter> TargetCharacter = nullptr;
	
	//애니메이션에서 사용할 조준 회전값
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Aim")
	FRotator AimRotation = FRotator::ZeroRotator;
	
	// 조준 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation")
	bool bIsAiming = false;

	//총구
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	TObjectPtr<USceneComponent> ProjectileSpawnPoint;
	
	//총알
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	TSubclassOf<AEnemyProjectile> ProjectileClass;
	
	UFUNCTION()
	void OnDetectionBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);
	
	UFUNCTION()
	void OnDetectionEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);
	
	FTimerHandle FireTimerHandle;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float FirstFireDelay = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	float FireInterval = 3.0f;
	
	virtual void Tick(float DeltaSeconds) override;
	
	//위로 반동
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	float RecoilAmount = 15.0f;

	//현재 적용할 반동값
	UPROPERTY(BlueprintReadOnly, Category = "Animation")
	float RecoilPitch = 0.0f;
};