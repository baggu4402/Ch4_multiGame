// Copyright SimpleVoiceChat Contributors. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "InputCoreTypes.h"
#include "SimpleVoiceChatSettings.generated.h"

/** Project Settings > Plugins > Simple Voice Chat. */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Simple Voice Chat"))
class SIMPLEVOICECHAT_API USimpleVoiceChatSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    USimpleVoiceChatSettings();

    virtual FName GetContainerName() const override;
    virtual FName GetCategoryName() const override;
    virtual FName GetSectionName() const override;

    /** Master switch for subsystem initialization and automatic input handling. */
    UPROPERTY(Config, EditAnywhere, Category="Voice")
    bool bEnableVoicePlugin = true;

    /** Toggles the microphone from the configured key while the game viewport has focus. */
    UPROPERTY(Config, EditAnywhere, Category="Input")
    bool bEnableAutomaticToggleKey = true;

    /** Default: T. The input preprocessor never consumes this key. */
    UPROPERTY(Config, EditAnywhere, Category="Input", meta=(EditCondition="bEnableAutomaticToggleKey"))
    FKey ToggleKey;

    /** Privacy-safe default is off. */
    UPROPERTY(Config, EditAnywhere, Category="Voice")
    bool bMicrophoneEnabledOnStart = false;
};
