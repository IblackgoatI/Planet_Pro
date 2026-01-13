#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "PlayFab.h"
#include "Core/PlayFabClientDataModels.h"
#include "Core/PlayFabClientAPI.h"
#include "PlayFabError.h" // 에러 처리를 위해 필요
#include "MyGameInstance.generated.h"

// 아이템 데이터 구조체
USTRUCT(BlueprintType)
struct FItemData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 Amount;
};

UCLASS()
class PLANET_PRO_API UMyGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// ==========================================================
	// 1. [기존] 인벤토리 시스템
	// ==========================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<FItemData> MyInventory;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddOrUpdateItem(FName InItemID, int32 InAmount);
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FString GetInventoryAsJsonString();
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	TMap<FString, FString> GetInventoryMapForPlayFab();
	
	UFUNCTION(BlueprintCallable, Category = "PlayFab")
	void SaveInventoryToPlayFab_CPP();


	// ==========================================================
	// 2. [NEW] 시간 저장/로드 시스템
	// ==========================================================

	// [시간 저장 시스템]
	UPROPERTY(BlueprintReadOnly, Category = "TimeSystem")
	float SavedSkyTime = -1.0f; 

	UFUNCTION(BlueprintCallable, Category = "TimeSystem")
	void SaveSkyTime(float CurrentTime);

	UFUNCTION(BlueprintCallable, Category = "TimeSystem")
	void LoadSkyTime();

private:
	// 콜백 함수들
	void OnSaveTimeSuccess(const PlayFab::ClientModels::FUpdateUserDataResult& Result);
	void OnLoadTimeSuccess(const PlayFab::ClientModels::FGetUserDataResult& Result);

	// ★★★ [수정됨] FPlayFabError -> FPlayFabCppError ★★★
	// 인자 타입을 FPlayFabCppError로 바꿔야 합니다.
	void OnTimeError(const PlayFab::FPlayFabCppError& ErrorResult);
};