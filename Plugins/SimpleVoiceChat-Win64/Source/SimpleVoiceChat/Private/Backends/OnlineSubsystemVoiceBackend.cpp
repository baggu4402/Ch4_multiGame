// Copyright SimpleVoiceChat Contributors. All Rights Reserved.

#include "Backends/OnlineSubsystemVoiceBackend.h"

#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "SimpleVoiceChatLog.h"

bool FOnlineSubsystemVoiceBackend::Initialize(UWorld* World)
{
    IOnlineSubsystem* NewOnlineSubsystem = World ? Online::GetSubsystem(World) : nullptr;
    IOnlineVoicePtr NewVoiceInterface = NewOnlineSubsystem ? NewOnlineSubsystem->GetVoiceInterface() : nullptr;

    if (OnlineSubsystem == NewOnlineSubsystem && VoiceInterface == NewVoiceInterface && VoiceInterface.IsValid())
    {
        return true;
    }

    ReleaseAllTalkers();
    OnlineSubsystem = NewOnlineSubsystem;
    VoiceInterface = MoveTemp(NewVoiceInterface);

    if (!VoiceInterface.IsValid())
    {
        if (!bWarnedVoiceUnavailable)
        {
            const FString BackendName = OnlineSubsystem
                ? OnlineSubsystem->GetSubsystemName().ToString()
                : TEXT("None");
            UE_LOG(
                LogSimpleVoiceChat,
                Warning,
                TEXT("Voice is unavailable. Active OnlineSubsystem: %s. Check the backend and voice configuration."),
                *BackendName);
            bWarnedVoiceUnavailable = true;
        }
        return false;
    }

    bWarnedVoiceUnavailable = false;
    UE_LOG(
        LogSimpleVoiceChat,
        Log,
        TEXT("OnlineSubsystem voice backend ready: %s"),
        *OnlineSubsystem->GetSubsystemName().ToString());
    return true;
}

void FOnlineSubsystemVoiceBackend::Shutdown()
{
    ReleaseAllTalkers();
    VoiceInterface.Reset();
    OnlineSubsystem = nullptr;
    bMicrophoneEnabled = false;
    bWarnedVoiceUnavailable = false;
}

void FOnlineSubsystemVoiceBackend::HandleWorldChanged(UWorld* NewWorld)
{
    // Remote PlayerStates are world-owned. Drop their registrations before rebuilding from the new world.
    UnregisterRemoteTalkers();
}

void FOnlineSubsystemVoiceBackend::RefreshTalkers(UWorld* World)
{
    if (!VoiceInterface.IsValid() || !World)
    {
        return;
    }

    RefreshLocalTalkers(World);
    RefreshRemoteTalkers(World);
}

bool FOnlineSubsystemVoiceBackend::SetMicrophoneEnabled(const bool bEnable)
{
    if (!VoiceInterface.IsValid())
    {
        return false;
    }

    if (bEnable && RegisteredLocalTalkers.IsEmpty())
    {
        return false;
    }

    bMicrophoneEnabled = bEnable;
    for (const uint32 LocalUserNum : RegisteredLocalTalkers)
    {
        ApplyMicrophoneStateToLocalTalker(LocalUserNum);
    }
    return true;
}

bool FOnlineSubsystemVoiceBackend::IsAvailable() const
{
    return VoiceInterface.IsValid();
}

bool FOnlineSubsystemVoiceBackend::IsMicrophoneEnabled() const
{
    return bMicrophoneEnabled;
}

bool FOnlineSubsystemVoiceBackend::IsLocalPlayerTalking() const
{
    if (!VoiceInterface.IsValid())
    {
        return false;
    }

    for (const uint32 LocalUserNum : RegisteredLocalTalkers)
    {
        if (VoiceInterface->IsLocalPlayerTalking(LocalUserNum))
        {
            return true;
        }
    }
    return false;
}

void FOnlineSubsystemVoiceBackend::RefreshLocalTalkers(UWorld* World)
{
    UGameInstance* GameInstance = World->GetGameInstance();
    if (!GameInstance)
    {
        return;
    }

    TSet<uint32> PresentLocalUsers;
    for (const ULocalPlayer* LocalPlayer : GameInstance->GetLocalPlayers())
    {
        if (!LocalPlayer)
        {
            continue;
        }

        const int32 ControllerId = LocalPlayer->GetControllerId();
        if (ControllerId < 0 || ControllerId > MAX_uint8)
        {
            UE_LOG(LogSimpleVoiceChat, Warning, TEXT("Ignoring unsupported local controller id: %d"), ControllerId);
            continue;
        }

        const uint32 LocalUserNum = static_cast<uint32>(ControllerId);
        PresentLocalUsers.Add(LocalUserNum);
        if (RegisteredLocalTalkers.Contains(LocalUserNum))
        {
            continue;
        }

        if (VoiceInterface->RegisterLocalTalker(LocalUserNum))
        {
            RegisteredLocalTalkers.Add(LocalUserNum);
            ApplyMicrophoneStateToLocalTalker(LocalUserNum);
            UE_LOG(LogSimpleVoiceChat, Log, TEXT("Registered local talker %u"), LocalUserNum);
        }
        else
        {
            UE_LOG(LogSimpleVoiceChat, Verbose, TEXT("Local talker %u is not ready; registration will be retried"), LocalUserNum);
        }
    }

    for (auto It = RegisteredLocalTalkers.CreateIterator(); It; ++It)
    {
        const uint32 LocalUserNum = *It;
        if (!PresentLocalUsers.Contains(LocalUserNum))
        {
            VoiceInterface->StopNetworkedVoice(static_cast<uint8>(LocalUserNum));
            VoiceInterface->UnregisterLocalTalker(LocalUserNum);
            UE_LOG(LogSimpleVoiceChat, Log, TEXT("Unregistered local talker %u"), LocalUserNum);
            It.RemoveCurrent();
        }
    }
}

void FOnlineSubsystemVoiceBackend::RefreshRemoteTalkers(UWorld* World)
{
    AGameStateBase* GameState = World->GetGameState();
    if (!GameState)
    {
        return;
    }

    TSet<FString> PresentRemoteTalkers;
    for (APlayerState* PlayerState : GameState->PlayerArray)
    {
        if (!IsValid(PlayerState) || PlayerState->IsInactive())
        {
            continue;
        }

        const APlayerController* PlayerController = PlayerState->GetPlayerController();
        if (PlayerController && PlayerController->IsLocalController())
        {
            continue;
        }

        const FUniqueNetIdRepl& UniqueId = PlayerState->GetUniqueId();
        if (!UniqueId.IsValid() || !UniqueId.IsV1())
        {
            continue;
        }

        const FString TalkerKey = MakeRemoteTalkerKey(UniqueId);
        PresentRemoteTalkers.Add(TalkerKey);
        if (RegisteredRemoteTalkers.Contains(TalkerKey))
        {
            continue;
        }

        if (VoiceInterface->RegisterRemoteTalker(*UniqueId))
        {
            RegisteredRemoteTalkers.Add(TalkerKey, UniqueId);
            UE_LOG(LogSimpleVoiceChat, Log, TEXT("Registered remote talker: %s"), *TalkerKey);
        }
        else
        {
            UE_LOG(LogSimpleVoiceChat, Verbose, TEXT("Remote talker is not ready; registration will be retried: %s"), *TalkerKey);
        }
    }

    for (auto It = RegisteredRemoteTalkers.CreateIterator(); It; ++It)
    {
        if (!PresentRemoteTalkers.Contains(It.Key()))
        {
            const FUniqueNetIdRepl& UniqueId = It.Value();
            if (UniqueId.IsValid() && UniqueId.IsV1())
            {
                VoiceInterface->UnregisterRemoteTalker(*UniqueId);
            }
            UE_LOG(LogSimpleVoiceChat, Log, TEXT("Unregistered remote talker: %s"), *It.Key());
            It.RemoveCurrent();
        }
    }
}

void FOnlineSubsystemVoiceBackend::UnregisterRemoteTalkers()
{
    if (VoiceInterface.IsValid())
    {
        for (const TPair<FString, FUniqueNetIdRepl>& Talker : RegisteredRemoteTalkers)
        {
            if (Talker.Value.IsValid() && Talker.Value.IsV1())
            {
                VoiceInterface->UnregisterRemoteTalker(*Talker.Value);
            }
        }
    }
    RegisteredRemoteTalkers.Reset();
}

void FOnlineSubsystemVoiceBackend::ReleaseAllTalkers()
{
    if (VoiceInterface.IsValid())
    {
        UnregisterRemoteTalkers();
        for (const uint32 LocalUserNum : RegisteredLocalTalkers)
        {
            VoiceInterface->StopNetworkedVoice(static_cast<uint8>(LocalUserNum));
            VoiceInterface->UnregisterLocalTalker(LocalUserNum);
        }
    }
    else
    {
        RegisteredRemoteTalkers.Reset();
    }
    RegisteredLocalTalkers.Reset();
}

void FOnlineSubsystemVoiceBackend::ApplyMicrophoneStateToLocalTalker(const uint32 LocalUserNum) const
{
    if (!VoiceInterface.IsValid() || LocalUserNum > MAX_uint8)
    {
        return;
    }

    if (bMicrophoneEnabled)
    {
        VoiceInterface->StartNetworkedVoice(static_cast<uint8>(LocalUserNum));
    }
    else
    {
        VoiceInterface->StopNetworkedVoice(static_cast<uint8>(LocalUserNum));
    }
}

FString FOnlineSubsystemVoiceBackend::MakeRemoteTalkerKey(const FUniqueNetIdRepl& UniqueId)
{
    return FString::Printf(TEXT("%s:%s"), *UniqueId.GetType().ToString(), *UniqueId.ToString());
}
