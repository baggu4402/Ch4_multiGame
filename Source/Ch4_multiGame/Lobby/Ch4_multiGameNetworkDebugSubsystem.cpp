// Copyright Epic Games, Inc. All Rights Reserved.

#include "Lobby/Ch4_multiGameNetworkDebugSubsystem.h"

#include "Ch4_multiGame.h"
#include "Engine/Engine.h"

void UCh4_multiGameNetworkDebugSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (GEngine)
	{
		NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(
			this,
			&UCh4_multiGameNetworkDebugSubsystem::HandleNetworkFailure);
		TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(
			this,
			&UCh4_multiGameNetworkDebugSubsystem::HandleTravelFailure);
	}
}

void UCh4_multiGameNetworkDebugSubsystem::Deinitialize()
{
	if (GEngine)
	{
		GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
		GEngine->OnTravelFailure().Remove(TravelFailureHandle);
	}

	Super::Deinitialize();
}

void UCh4_multiGameNetworkDebugSubsystem::HandleNetworkFailure(
	UWorld* World,
	UNetDriver* NetDriver,
	const ENetworkFailure::Type FailureType,
	const FString& ErrorString)
{
	const FString FailureName = ENetworkFailure::ToString(FailureType);
	const FString Details = FString::Printf(
		TEXT("%s: %s\nCheck the exact IP:port, Hamachi adapter, and UDP firewall rule."),
		*FailureName,
		ErrorString.IsEmpty() ? TEXT("No error details") : *ErrorString);

	UE_LOG(LogCh4_multiGame, Error,
		TEXT("[NetworkDebug] NETWORK FAILURE | Type: %s | Error: %s | World: %s | Driver: %s"),
		*FailureName,
		ErrorString.IsEmpty() ? TEXT("No error details") : *ErrorString,
		*GetNameSafe(World),
		*GetNameSafe(NetDriver));
	ShowFailureMessage(TEXT("NETWORK CONNECTION FAILED"), Details);
}

void UCh4_multiGameNetworkDebugSubsystem::HandleTravelFailure(
	UWorld* World,
	const ETravelFailure::Type FailureType,
	const FString& ErrorString)
{
	const FString FailureName = ETravelFailure::ToString(FailureType);
	const FString Details = FString::Printf(
		TEXT("%s: %s\nCheck the map name. Lobby map: /Game/Lobby/L_Lobby"),
		*FailureName,
		ErrorString.IsEmpty() ? TEXT("No error details") : *ErrorString);

	UE_LOG(LogCh4_multiGame, Error,
		TEXT("[NetworkDebug] TRAVEL FAILURE | Type: %s | Error: %s | World: %s"),
		*FailureName,
		ErrorString.IsEmpty() ? TEXT("No error details") : *ErrorString,
		*GetNameSafe(World));
	ShowFailureMessage(TEXT("MAP TRAVEL FAILED"), Details);
}

void UCh4_multiGameNetworkDebugSubsystem::ShowFailureMessage(
	const FString& Title,
	const FString& Details) const
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 20.0f, FColor::Red, FString::Printf(
			TEXT("[%s]\n%s\nSee Output Log: [NetworkDebug]"),
			*Title,
			*Details));
	}
}
