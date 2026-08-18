#include "UI/MainMenu/Ch4MainMenuViewModel.h"

#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Kismet/KismetSystemLibrary.h"

// 패널 전환
void UCh4MainMenuViewModel::ShowRoomSelection()
{
	SetCurrentPanel(EMenuPanel::RoomSelection);
}

void UCh4MainMenuViewModel::ShowMainMenu()
{
	SetCurrentPanel(EMenuPanel::Main);
	SetStatusText(FText::GetEmpty());
}

// 세션 액션
void UCh4MainMenuViewModel::HostGame()
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}
	
	SetbIsLoading(true);
	SetStatusText(NSLOCTEXT("Ch4MainMenu", "HostingGame", "Creating room..."));
	
	// Listen Server로 L_Lobby 맵 이동
	// ?listen = 이 PC가 서버 역할을 맡음
	APlayerController* PC = World->GetFirstPlayerController();
	if (IsValid(PC))
	{
		PC->ClientTravel(TEXT("/Game/Lobby/L_Lobby?listen"), TRAVEL_Absolute);
	}
}

void UCh4MainMenuViewModel::FindRooms()
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (!Subsystem)
	{
		SetStatusText(NSLOCTEXT("Ch4MainMenu", "NoSubsystem", "Online subsystem not found."));
		return;
	}
	
	IOnlineSessionPtr Session = Subsystem->GetSessionInterface();
	if (!Session.IsValid())
	{
		return;
	}
	
	SetbIsLoading(true);
	SetStatusText(NSLOCTEXT("Ch4MainMenu", "SearchingRooms", "Searching for rooms..."));
	SetCurrentPanel(EMenuPanel::RoomList);
	
	// 세션 검색 설정
	SearchSettings = MakeShareable(new FOnlineSessionSearch());
	SearchSettings->bIsLanQuery = true;		// LAN 검색 (Steam 전환 시 false로 변경)
	SearchSettings->MaxSearchResults = 20;
	
	// 검색 완료 콜백 등록 (구독)
	Session->OnFindSessionsCompleteDelegates.AddUObject(
		this, &UCh4MainMenuViewModel::OnFindSessionsComplete);
	
	Session->FindSessions(0, SearchSettings.ToSharedRef());
}



void UCh4MainMenuViewModel::OnFindSessionsComplete(bool bWasSuccessful)
{
	SetbIsLoading(false);
	
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (!Subsystem)
	{
		return;
	}
	
	// 콜백 구독 해제 (중복 호출 방지)
	IOnlineSessionPtr Session = Subsystem->GetSessionInterface();
	if (Session.IsValid())
	{
		Session->OnFindSessionsCompleteDelegates.RemoveAll(this);
	}
	
	if (!bWasSuccessful || !SearchSettings.IsValid())
	{
		SetStatusText(NSLOCTEXT("Ch4MainMenu", "SearchFailed", "Search failed. Please try again."));
		return;
	}
	
	const int32 ResultCount = SearchSettings->SearchResults.Num();
	if (ResultCount == 0)
	{
		SetStatusText(NSLOCTEXT("Ch4MainMenu", "NoRooms", "No rooms found."));
		return;
	}
	
	SetStatusText(FText::Format(NSLOCTEXT("Ch4MainMenu", "RoomsFound", "Found {0} room(s)"), FText::AsNumber(ResultCount)));
}

// 시스템
void UCh4MainMenuViewModel::QuitGame()
{
	UKismetSystemLibrary::QuitGame(
		GetWorld(),
		nullptr,
		EQuitPreference::Quit,
		false);
}

// Private Setters (MVVM 내부 규칙)
void UCh4MainMenuViewModel::SetCurrentPanel(EMenuPanel NewPanel)
{
	if (CurrentPanel != NewPanel)
	{
		CurrentPanel = NewPanel;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CurrentPanel);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(MainPanelVisibility);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(RoomSelectionVisibility);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(RoomListVisibility);
	}
}

void UCh4MainMenuViewModel::SetStatusText(FText NewText)
{
	UE_MVVM_SET_PROPERTY_VALUE(StatusText, NewText);
}

void UCh4MainMenuViewModel::SetbIsLoading(bool bNewIsLoading)
{
	if (bIsLoading != bNewIsLoading)
	{
		bIsLoading = bNewIsLoading;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bIsLoading);
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bCanInteract);
	}
}
