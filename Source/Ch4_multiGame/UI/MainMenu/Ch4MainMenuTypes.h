#pragma once

#include "CoreMinimal.h"
#include "Ch4MainMenuTypes.generated.h"

// 메인 메뉴에서 현재 표시 중인 패널 상태
UENUM(BlueprintType)
enum class EMenuPanel : uint8
{
	Main			UMETA(DisplayName = "Main"),
	RoomSelection	UMETA(DisplayName = "Room Selection"),
	RoomList		UMETA(DisplayName = "Room List"),
};