// Copyright Epic Games, Inc. All Rights Reserved.

#include "Lobby/Ch4_multiGameLobbyGameMode.h"

#include "Ch4_multiGame.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/PlayerState.h"
#include "Lobby/Ch4_multiGameLobbyGameState.h"
#include "UObject/ConstructorHelpers.h"

ACh4_multiGameLobbyGameMode::ACh4_multiGameLobbyGameMode()
{
	GameStateClass = ACh4_multiGameLobbyGameState::StaticClass();

	static ConstructorHelpers::FClassFinder<APawn> ThirdPersonPawnClass(
		TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (ThirdPersonPawnClass.Succeeded())
	{
		DefaultPawnClass = ThirdPersonPawnClass.Class;
	}

	static ConstructorHelpers::FClassFinder<APlayerController> ThirdPersonControllerClass(
		TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonPlayerController"));
	if (ThirdPersonControllerClass.Succeeded())
	{
		PlayerControllerClass = ThirdPersonControllerClass.Class;
	}
}

void ACh4_multiGameLobbyGameMode::InitGame(
	const FString& MapName,
	const FString& Options,
	FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	MaxLobbyPlayers = FMath::Max(MaxLobbyPlayers, 1);
	if (GameSession)
	{
		// AGameSession::ApproveLogin uses this value before Login/PostLogin.
		GameSession->MaxPlayers = MaxLobbyPlayers;
	}

	UE_LOG(LogCh4_multiGame, Log, TEXT("[Lobby] Initialized with a maximum of %d players"), MaxLobbyPlayers);
}

void ACh4_multiGameLobbyGameMode::InitGameState()
{
	Super::InitGameState();
	UpdateLobbyPlayerCount(GetNumPlayers());
}

void ACh4_multiGameLobbyGameMode::PreLogin(
	const FString& Options,
	const FString& Address,
	const FUniqueNetIdRepl& UniqueId,
	FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	if (ErrorMessage.IsEmpty() && GetNumPlayers() >= MaxLobbyPlayers)
	{
		ErrorMessage = FString::Printf(TEXT("Lobby is full (%d/%d)."), GetNumPlayers(), MaxLobbyPlayers);
	}

	if (!ErrorMessage.IsEmpty())
	{
		UE_LOG(LogCh4_multiGame, Warning,
			TEXT("[Lobby] Connection Rejected from %s: %s Players: %d / %d"),
			*Address,
			*ErrorMessage,
			GetNumPlayers(),
			MaxLobbyPlayers);
	}
}

void ACh4_multiGameLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!HasAuthority() || !IsValid(NewPlayer))
	{
		return;
	}

	UpdateLobbyPlayerCount(GetNumPlayers());
	UE_LOG(LogCh4_multiGame, Log, TEXT("[Lobby] Player Joined: %s"), *GetPlayerLogLabel(NewPlayer));
	UE_LOG(LogCh4_multiGame, Log, TEXT("[Lobby] Players: %d / %d"), GetNumPlayers(), MaxLobbyPlayers);
}

void ACh4_multiGameLobbyGameMode::Logout(AController* Exiting)
{
	const FString PlayerLabel = GetPlayerLogLabel(Exiting);
	const bool bWasPlayerController = IsValid(Cast<APlayerController>(Exiting));
	const int32 RemainingPlayerCount = bWasPlayerController
		? FMath::Max(GetNumPlayers() - 1, 0)
		: GetNumPlayers();

	Super::Logout(Exiting);

	if (!HasAuthority() || !bWasPlayerController)
	{
		return;
	}

	UpdateLobbyPlayerCount(RemainingPlayerCount);
	UE_LOG(LogCh4_multiGame, Log, TEXT("[Lobby] Player Left: %s"), *PlayerLabel);
	UE_LOG(LogCh4_multiGame, Log, TEXT("[Lobby] Players: %d / %d"), RemainingPlayerCount, MaxLobbyPlayers);
}

ACh4_multiGameLobbyGameState* ACh4_multiGameLobbyGameMode::GetLobbyGameState() const
{
	return GetWorld() ? GetWorld()->GetGameState<ACh4_multiGameLobbyGameState>() : nullptr;
}

void ACh4_multiGameLobbyGameMode::UpdateLobbyPlayerCount(const int32 NewPlayerCount)
{
	if (ACh4_multiGameLobbyGameState* LobbyGameState = GetLobbyGameState())
	{
		LobbyGameState->SetPlayerCounts(NewPlayerCount, MaxLobbyPlayers);
	}
	else
	{
		UE_LOG(LogCh4_multiGame, Error,
			TEXT("[Lobby] ACh4_multiGameLobbyGameState is not active. Check the Lobby GameMode assignment."));
	}
}

FString ACh4_multiGameLobbyGameMode::GetPlayerLogLabel(const AController* Controller) const
{
	if (!IsValid(Controller))
	{
		return TEXT("UnknownPlayer");
	}

	if (const APlayerState* PlayerState = Controller->GetPlayerState<APlayerState>())
	{
		const FString PlayerName = PlayerState->GetPlayerName();
		return PlayerName.IsEmpty()
			? GetNameSafe(Controller)
			: FString::Printf(TEXT("%s (%s)"), *PlayerName, *GetNameSafe(Controller));
	}

	return GetNameSafe(Controller);
}
