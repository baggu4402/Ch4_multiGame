// Copyright SimpleVoiceChat Contributors. All Rights Reserved.

#include "SimpleVoiceChatSubsystem.h"

#include "Backends/OnlineSubsystemVoiceBackend.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "SimpleVoiceChatLog.h"
#include "SimpleVoiceChatSettings.h"
#include "UObject/UObjectGlobals.h"

USimpleVoiceChatSubsystem::~USimpleVoiceChatSubsystem() = default;

void USimpleVoiceChatSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
        this,
        &USimpleVoiceChatSubsystem::HandlePostLoadMap);

    const USimpleVoiceChatSettings* Settings = GetDefault<USimpleVoiceChatSettings>();
    if (!Settings->bEnableVoicePlugin)
    {
        UE_LOG(LogSimpleVoiceChat, Log, TEXT("Subsystem created with voice disabled in project settings"));
        return;
    }

    const bool bEnableOnStart = Settings->bMicrophoneEnabledOnStart;
    InitializeVoice();
    if (bEnableOnStart)
    {
        EnableMicrophone();
    }
}

void USimpleVoiceChatSubsystem::Deinitialize()
{
    if (PostLoadMapHandle.IsValid())
    {
        FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
        PostLoadMapHandle.Reset();
    }

    ShutdownVoice();
    Super::Deinitialize();
}

bool USimpleVoiceChatSubsystem::InitializeVoice()
{
    const USimpleVoiceChatSettings* Settings = GetDefault<USimpleVoiceChatSettings>();
    if (!Settings->bEnableVoicePlugin)
    {
        return false;
    }

    if (!VoiceBackend)
    {
        VoiceBackend = MakeUnique<FOnlineSubsystemVoiceBackend>();
    }

    bVoiceInitialized = true;
    UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
    StartRefreshTimer(World);

    if (!VoiceBackend->Initialize(World))
    {
        return false;
    }

    VoiceBackend->RefreshTalkers(World);
    VoiceBackend->SetMicrophoneEnabled(bMicrophoneEnabled);
    return VoiceBackend->IsAvailable();
}

void USimpleVoiceChatSubsystem::ShutdownVoice()
{
    StopRefreshTimer();

    if (VoiceBackend)
    {
        VoiceBackend->Shutdown();
        VoiceBackend.Reset();
    }

    bVoiceInitialized = false;
    if (bMicrophoneEnabled)
    {
        bMicrophoneEnabled = false;
        OnMicrophoneStateChanged.Broadcast(false);
    }
}

bool USimpleVoiceChatSubsystem::EnableMicrophone()
{
    return ApplyMicrophoneState(true);
}

bool USimpleVoiceChatSubsystem::DisableMicrophone()
{
    return ApplyMicrophoneState(false);
}

bool USimpleVoiceChatSubsystem::ToggleMicrophone()
{
    return ApplyMicrophoneState(!bMicrophoneEnabled);
}

bool USimpleVoiceChatSubsystem::IsMicrophoneEnabled() const
{
    return bMicrophoneEnabled;
}

bool USimpleVoiceChatSubsystem::IsVoiceAvailable() const
{
    return VoiceBackend && VoiceBackend->IsAvailable();
}

bool USimpleVoiceChatSubsystem::IsLocalPlayerTalking() const
{
    return VoiceBackend && VoiceBackend->IsLocalPlayerTalking();
}

void USimpleVoiceChatSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
    if (!bVoiceInitialized || !LoadedWorld || LoadedWorld->GetGameInstance() != GetGameInstance())
    {
        return;
    }

    if (VoiceBackend)
    {
        VoiceBackend->HandleWorldChanged(LoadedWorld);
    }
    StartRefreshTimer(LoadedWorld);
    RefreshVoiceState();
}

void USimpleVoiceChatSubsystem::RefreshVoiceState()
{
    if (!bVoiceInitialized || !VoiceBackend)
    {
        return;
    }

    UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
    if (!VoiceBackend->Initialize(World))
    {
        return;
    }

    VoiceBackend->RefreshTalkers(World);
    if (VoiceBackend->IsMicrophoneEnabled() != bMicrophoneEnabled)
    {
        VoiceBackend->SetMicrophoneEnabled(bMicrophoneEnabled);
    }
}

void USimpleVoiceChatSubsystem::StartRefreshTimer(UWorld* World)
{
    StopRefreshTimer();
    if (!World)
    {
        return;
    }

    RefreshTimerWorld = World;
    World->GetTimerManager().SetTimer(
        RefreshTimerHandle,
        this,
        &USimpleVoiceChatSubsystem::RefreshVoiceState,
        1.0f,
        true,
        0.25f);
}

void USimpleVoiceChatSubsystem::StopRefreshTimer()
{
    if (UWorld* TimerWorld = RefreshTimerWorld.Get())
    {
        TimerWorld->GetTimerManager().ClearTimer(RefreshTimerHandle);
    }
    RefreshTimerWorld.Reset();
    RefreshTimerHandle.Invalidate();
}

bool USimpleVoiceChatSubsystem::ApplyMicrophoneState(const bool bEnable)
{
    if (!bVoiceInitialized && !InitializeVoice())
    {
        return false;
    }

    RefreshVoiceState();
    if (!VoiceBackend || !VoiceBackend->SetMicrophoneEnabled(bEnable))
    {
        UE_LOG(
            LogSimpleVoiceChat,
            Warning,
            TEXT("Cannot %s the microphone because no local voice talker is available"),
            bEnable ? TEXT("enable") : TEXT("disable"));
        return false;
    }

    if (bMicrophoneEnabled != bEnable)
    {
        bMicrophoneEnabled = bEnable;
        UE_LOG(LogSimpleVoiceChat, Log, TEXT("Microphone %s"), bEnable ? TEXT("enabled") : TEXT("disabled"));
        OnMicrophoneStateChanged.Broadcast(bMicrophoneEnabled);
    }
    return true;
}
