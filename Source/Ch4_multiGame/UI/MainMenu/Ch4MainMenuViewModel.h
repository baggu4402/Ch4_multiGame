#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "OnlineSessionSettings.h"
#include "UI/MainMenu/Ch4MainMenuTypes.h"
#include "Ch4MainMenuViewModel.generated.h"

// 메인 메뉴 전체를 제어하는 ViewModel
// 패널 전환, 방 만들기/찾기, 게임 종료 로직을 담당
UCLASS(BlueprintType)
class UCh4MainMenuViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()
	
public:
	// FieldNotify 속성 - View Binding 연결 대상
	// 현재 보여줄 패널 (Main / RoomSelection / RoomList)
	UPROPERTY(FieldNotify, Setter, Getter, BlueprintReadOnly, Category = "Menu|State")
	EMenuPanel CurrentPanel = EMenuPanel::Main;
	
	// 버튼 아래 표시되는 상태 텍스트 ("방 생성중...", "검색 중..." 등)
	UPROPERTY(FieldNotify, Setter, Getter, BlueprintReadOnly, Category = "Menu|State")
	FText StatusText;
	
	// true일 때 방 만들기/찾기 버튼 비활성화
	UPROPERTY(FieldNotify, Setter, Getter, BlueprintReadOnly, Category = "Menu|State")
	bool bIsLoading = false;
	
	// 버튼이 호출할 함수들
	// [게임 시작] 버튼 -> RoomSelection 패널로 전환
	UFUNCTION(BlueprintCallable, Category = "Menu|Navigation")
	void ShowRoomSelection();
	
	// [뒤로 가기] 버튼 -> Main 패널로 복귀
	UFUNCTION(BlueprintCallable, Category = "Menu|Navigation")
	void ShowMainMenu();
	
	// [방 만들기] 버튼 -> 호스트로 L_Lobby 이동
	UFUNCTION(BlueprintCallable, Category = "Menu|Session")
	void HostGame();
	
	// [방 찾기] 버튼 -> LAN 세션 검색
	UFUNCTION(BlueprintCallable, Category = "Menu|Session")
	void FindRooms();
	
	UFUNCTION(BlueprintCallable, Category = "Menu|System")
	void QuitGame();
	
private:
	// FieldNotify Setter/Getter (MVVM 내부 규칙)
	void SetCurrentPanel(EMenuPanel NewPanel);
	EMenuPanel GetCurrentPanel() const { return CurrentPanel; }
	
	void SetStatusText(FText NewText);
	FText GetStatusText() const { return StatusText; }
	
	void SetbIsLoading(bool bNewIsLoading);
	bool GetbIsLoading() const { return bIsLoading; }
	
	// FindRooms 완료 시 호출되는 콜백
	void OnFindSessionsComplete(bool bWasSuccessful);
	
	// 세션 검색 설정 보관용
	TSharedPtr<FOnlineSessionSearch> SearchSettings;
};