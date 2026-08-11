// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameFlow/FinalDeliveryZone.h"

#include "Ch4_multiGame.h"
#include "Ch4_multiGameGameMode.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "GameFlow/GameFlowTargetInterface.h"
#include "Engine/World.h"

AFinalDeliveryZone::AFinalDeliveryZone()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	TriggerCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerCollision"));
	TriggerCollision->SetupAttachment(SceneRoot);
	TriggerCollision->SetBoxExtent(FVector(300.0f, 300.0f, 150.0f));
	TriggerCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerCollision->SetCollisionObjectType(ECC_WorldDynamic);
	TriggerCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	TriggerCollision->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
	TriggerCollision->SetGenerateOverlapEvents(true);
	TriggerCollision->OnComponentBeginOverlap.AddDynamic(this, &AFinalDeliveryZone::OnTriggerBeginOverlap);
}

void AFinalDeliveryZone::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !IsValid(OtherActor) || OtherActor == this)
	{
		return;
	}

	if (!OtherActor->GetClass()->ImplementsInterface(UGameFlowTargetInterface::StaticClass()))
	{
		return;
	}

	UE_LOG(LogCh4_multiGame, Log, TEXT("[GameFlow] Final Delivery target entered the zone: %s"), *GetNameSafe(OtherActor));

	if (ACh4_multiGameGameMode* GameMode = GetWorld()->GetAuthGameMode<ACh4_multiGameGameMode>())
	{
		GameMode->TryCompleteGame();
	}
	else
	{
		UE_LOG(LogCh4_multiGame, Error, TEXT("[GameFlow] FinalDeliveryZone requires ACh4_multiGameGameMode"));
	}
}
