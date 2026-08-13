// Copyright SimpleVoiceChat Contributors. All Rights Reserved.

#pragma once

#include "ISimpleVoiceBackend.h"
#include "GameFramework/OnlineReplStructs.h"
#include "Interfaces/VoiceInterface.h"

class IOnlineSubsystem;

/** Legacy IOnlineVoice adapter supplied by the currently active OnlineSubsystem. */
class FOnlineSubsystemVoiceBackend final : public ISimpleVoiceBackend
{
public:
    virtual bool Initialize(UWorld* World) override;
    virtual void Shutdown() override;
    virtual void HandleWorldChanged(UWorld* NewWorld) override;
    virtual void RefreshTalkers(UWorld* World) override;
    virtual bool SetMicrophoneEnabled(bool bEnable) override;
    virtual bool IsAvailable() const override;
    virtual bool IsMicrophoneEnabled() const override;
    virtual bool IsLocalPlayerTalking() const override;

private:
    void RefreshLocalTalkers(UWorld* World);
    void RefreshRemoteTalkers(UWorld* World);
    void UnregisterRemoteTalkers();
    void ReleaseAllTalkers();
    void ApplyMicrophoneStateToLocalTalker(uint32 LocalUserNum) const;
    static FString MakeRemoteTalkerKey(const FUniqueNetIdRepl& UniqueId);

    IOnlineSubsystem* OnlineSubsystem = nullptr;
    IOnlineVoicePtr VoiceInterface;
    TSet<uint32> RegisteredLocalTalkers;
    TMap<FString, FUniqueNetIdRepl> RegisteredRemoteTalkers;
    bool bMicrophoneEnabled = false;
    bool bWarnedVoiceUnavailable = false;
};
