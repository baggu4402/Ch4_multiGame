#include "Public/Map/LevelFloorBase.h"
#include "Components/BoxComponent.h"

ALevelFloorBase::ALevelFloorBase()
{
	PrimaryActorTick.bCanEverTick = false;
	
	bReplicates=true;
	SetReplicateMovement(true);

	// 루트 컴포넌트 기본 설정
	USceneComponent* RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	SetRootComponent(RootComp);

	// 시작점, 끝점 씬 컴포넌트 생성
	StartPoint = CreateDefaultSubobject<USceneComponent>(TEXT("StartPoint"));
	StartPoint->SetupAttachment(RootComponent);

	EndPoint = CreateDefaultSubobject<USceneComponent>(TEXT("EndPoint"));
	EndPoint->SetupAttachment(RootComponent);

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetupAttachment(RootComponent);
	CollisionBox->SetCollisionObjectType(ECC_WorldStatic);
}


void ALevelFloorBase::BeginPlay()
{
	Super::BeginPlay();
	
}

