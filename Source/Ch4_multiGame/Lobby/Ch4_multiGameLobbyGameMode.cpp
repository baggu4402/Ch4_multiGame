// Copyright Epic Games, Inc. All Rights Reserved.

#include "Lobby/Ch4_multiGameLobbyGameMode.h"

#include "Ch4_multiGame.h"
#include "Engine/Engine.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/PlayerState.h"
#include "IPAddress.h"
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

	const bool bIsListenServer = GetNetMode() == NM_ListenServer;
	const int32 ListenPort = GetListenPort();
	const FString NetworkMode = bIsListenServer ? TEXT("LISTEN SERVER") : TEXT("STANDALONE");
	const FString StartupMessage = bIsListenServer
		? FString::Printf(
			TEXT("%s READY | Port: %d | Client: open <HostHamachiIP>:%d"),
			*NetworkMode,
			ListenPort,
			ListenPort)
		: TEXT("STANDALONE ONLY | Clients cannot join | Run: open L_Lobby?listen");

	UE_LOG(LogCh4_multiGame, Log,
		TEXT("[Lobby] %s | Map: %s | MaxPlayers: %d"),
		*StartupMessage,
		*MapName,
		MaxLobbyPlayers);
	ShowServerDebugStatus(StartupMessage, bIsListenServer ? FColor::Green : FColor::Red, 30.0f);
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
	const FString PlayerLabel = GetPlayerLogLabel(NewPlayer);
	const FString PawnLabel = GetNameSafe(NewPlayer->GetPawn());
	UE_LOG(LogCh4_multiGame, Log,
		TEXT("[Lobby] Player Joined: %s | Pawn: %s | Possessed: %s"),
		*PlayerLabel,
		*PawnLabel,
		NewPlayer->GetPawn() ? TEXT("YES") : TEXT("NO"));
	UE_LOG(LogCh4_multiGame, Log, TEXT("[Lobby] Players: %d / %d"), GetNumPlayers(), MaxLobbyPlayers);
	ShowServerDebugStatus(
		FString::Printf(
			TEXT("PLAYER JOINED\nPlayers: %d / %d\nPawn: %s"),
			GetNumPlayers(),
			MaxLobbyPlayers,
			NewPlayer->GetPawn() ? TEXT("OK") : TEXT("MISSING")),
		NewPlayer->GetPawn() ? FColor::Green : FColor::Red,
		12.0f);
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
	ShowServerDebugStatus(
		FString::Printf(TEXT("PLAYER LEFT\nPlayers: %d / %d"), RemainingPlayerCount, MaxLobbyPlayers),
		FColor::Yellow,
		10.0f);
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

void ACh4_multiGameLobbyGameMode::ShowServerDebugStatus(
	const FString& EventMessage,
	const FColor& Color,
	const float Duration) const
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, Duration, Color, FString::Printf(
			TEXT("[LOBBY SERVER]\n%s"),
			*EventMessage));
	}
}

int32 ACh4_multiGameLobbyGameMode::GetListenPort() const
{
	if (const UWorld* World = GetWorld())
	{
		if (UNetDriver* NetDriver = World->GetNetDriver())
		{
			if (const TSharedPtr<const FInternetAddr> LocalAddress = NetDriver->GetLocalAddr())
			{
				return LocalAddress->GetPort();
			}
		}

		return World->URL.Port;
	}

	return 0;
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
