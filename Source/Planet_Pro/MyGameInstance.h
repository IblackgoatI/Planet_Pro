// MyGameInstance.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "PlayFab.h"
#include "Core/PlayFabClientDataModels.h"
#include "Core/PlayFabClientAPI.h"
#include "PlayFabError.h" 
#include "MyGameInstance.generated.h"

// 시간 로드 완료 알림용 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSkyTimeLoaded, float, LoadedTime);

// 아이템 데이터 구조체
USTRUCT(BlueprintType)
struct FItemData
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    FName ItemID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    int32 Amount = 0;
};

UCLASS()
class PLANET_PRO_API UMyGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    // ==========================================================
    // 1. 인벤토리 시스템
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
    // 2. 시간 저장/로드 시스템
    // ==========================================================

    // [중요] 저장된 시간 변수 (기본값 -1.0)
    UPROPERTY(BlueprintReadOnly, Category = "TimeSystem")
    float SavedSkyTime = -1.0f; 

    UFUNCTION(BlueprintCallable, Category = "TimeSystem")
    void SaveSkyTime(float CurrentTime);

    UFUNCTION(BlueprintCallable, Category = "TimeSystem")
    void LoadSkyTime();

    // "시간 로드 완료되면 알려줄게!" 하는 방송국
    UPROPERTY(BlueprintAssignable, Category = "PlayFab")
    FOnSkyTimeLoaded OnSkyTimeLoaded;

private:
    // 콜백 함수들
    void OnSaveTimeSuccess(const PlayFab::ClientModels::FUpdateUserDataResult& Result);
    void OnLoadTimeSuccess(const PlayFab::ClientModels::FGetUserDataResult& Result);
    void OnTimeError(const PlayFab::FPlayFabCppError& ErrorResult);
};