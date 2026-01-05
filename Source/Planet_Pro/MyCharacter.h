#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Planet_ProTypes.h"
#include "MainHUDWidget.h"

// [수정] Core/ 를 붙여서 경로를 명확하게 지정해야 함!
#include "PlayFab.h"
#include "PlayFabError.h"
#include "Core/PlayFabClientDataModels.h" // <--- [Core/ 추가]
#include "Core/PlayFabClientAPI.h"        // <--- [Core/ 추가]

#include "MyCharacter.generated.h"

UCLASS()
class PLANET_PRO_API AMyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMyCharacter();

protected:
	virtual void BeginPlay() override;

public:
	// UI 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUserWidget> MainHUDClass;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	class UMainHUDWidget* MainHUDInstance;

	// 인벤토리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<FPlanetItemInfo> Inventory;

	// 함수들
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void OnInventoryKeyPressed();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddTestItem();

	// PlayFab 저장 함수들
	void SaveInventoryToPlayFab();
	void OnSaveSuccess(const PlayFab::ClientModels::FUpdateUserDataResult& Result);
	void OnSaveError(const PlayFab::FPlayFabCppError& ErrorResult);
	// [추가] PlayFab에서 인벤토리 가져오기
	void LoadInventoryFromPlayFab();
	// [추가] 불러오기 성공 시 처리
	void OnLoadSuccess(const PlayFab::ClientModels::FGetUserDataResult& Result);\
	// [추가] 불러오기 실패 시 처리
	void OnLoadError(const PlayFab::FPlayFabCppError& ErrorResult);
};