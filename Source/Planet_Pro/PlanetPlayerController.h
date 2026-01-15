// PlanetPlayerController.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PlanetPlayerController.generated.h"

UCLASS()
class PLANET_PRO_API APlanetPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	// =================================================================
	// [1] 언리얼 라이프사이클 함수 (Protected가 맞습니다)
	// =================================================================
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override; // [New] 키 입력 설정을 위해 추가


	// =================================================================
	// [2] UI (ESC 메뉴) 관련 변수 및 함수
	// =================================================================
	
	// 에디터에서 WBP_PauseMenu를 지정할 변수 (C++ BP에서 설정)
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UUserWidget> PauseMenuClass;

	// 현재 화면에 떠 있는 위젯을 기억하는 변수
	UPROPERTY()
	class UUserWidget* PauseMenuInstance;

	// ESC 키를 눌렀을 때 실행할 함수
	void TogglePauseMenu();

public:
	// =================================================================
	// [3] 하늘/시간 관련 함수 (기존 기능)
	// =================================================================
	
	// 하늘 시간을 강제로 '꽂아넣는' 함수
	void ForceSkyUpdate();

	// 준비 끝났으니 화면 밝히는 함수
	void FadeInScreen();
	
	// PlayFab 데이터 로드 완료 시 호출될 콜백
	UFUNCTION() 
	void OnSkyTimeLoadedReceived(float LoadedTime);
};