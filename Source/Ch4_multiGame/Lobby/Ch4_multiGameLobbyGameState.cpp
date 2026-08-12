// Copyright Epic Games, Inc. All Rights Reserved.

#include "Lobby/Ch4_multiGameLobbyGameState.h"

#include "Ch4_multiGame.h"
#include "Net/UnrealNetwork.h"

void ACh4_multiGameLobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACh4_multiGameLobbyGameState, CurrentPlayerCount);
	DOREPLIFETIME(ACh4_multiGameLobbyGameState, MaxPlayerCount);
}

bool ACh4_multiGameLobbyGameState::SetPlayerCounts(
	const int32 NewCurrentPlayerCount,
	const int32 NewMaxPlayerCount)
{
	if (!HasAuthority())
	{
		UE_LOG(LogCh4_multiGame, Warning, TEXT("[Lobby] Rejected a client-side player-count change"));
		return false;
	}

	const int32 ValidatedMaxPlayerCount = FMath::Max(NewMaxPlayerCount, 1);
	const int32 ValidatedCurrentPlayerCount = FMath::Clamp(
		NewCurrentPlayerCount,
		0,
		ValidatedMaxPlayerCount);

	if (CurrentPlayerCount == ValidatedCurrentPlayerCount
		&& MaxPlayerCount == ValidatedMaxPlayerCount)
	{
		return false;
	}

	CurrentPlayerCount = ValidatedCurrentPlayerCount;
	MaxPlayerCount = ValidatedMaxPlayerCount;
	BroadcastPlayerCountChanged();
	ForceNetUpdate();
	return true;
}

void ACh4_multiGameLobbyGameState::OnRep_CurrentPlayerCount()
{
	UE_LOG(LogCh4_multiGame, Log,
		TEXT("[Lobby] Replicated Players: %d / %d"),
		CurrentPlayerCount,
		MaxPlayerCount);
	BroadcastPlayerCountChanged();
}

void ACh4_multiGameLobbyGameState::BroadcastPlayerCountChanged()
{
	OnPlayerCountChanged.Broadcast(CurrentPlayerCount, MaxPlayerCount);
}
