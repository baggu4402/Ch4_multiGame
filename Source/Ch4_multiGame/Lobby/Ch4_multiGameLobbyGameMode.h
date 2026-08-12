// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Ch4_multiGameLobbyGameMode.generated.h"

/** Server-authoritative rules owner for the direct-IP multiplayer test lobby. */
UCLASS()
class ACh4_multiGameLobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACh4_multiGameLobbyGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void InitGameState() override;
	virtual void PreLogin(
		const FString& Options,
		const FString& Address,
		const FUniqueNetIdRepl& UniqueId,
		FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	UFUNCTION(BlueprintPure, Category="Lobby")
	int32 GetMaxLobbyPlayers() const { return MaxLobbyPlayers; }

protected:
	/** Single source of truth for the lobby capacity, including the listen-server host. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Lobby", meta=(ClampMin="1", UIMin="1"))
	int32 MaxLobbyPlayers = 4;

private:
	class ACh4_multiGameLobbyGameState* GetLobbyGameState() const;
	void UpdateLobbyPlayerCount(int32 NewPlayerCount);
	FString GetPlayerLogLabel(const AController* Controller) const;
};
