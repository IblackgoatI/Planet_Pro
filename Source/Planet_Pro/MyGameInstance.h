// MyGameInstance.h

#pragma once

#include "CoreMinimal.h"
#include "AdvancedFriendsGameInstance.h"
#include "Engine/GameInstance.h"
#include "PlayFab.h"
#include "Core/PlayFabClientDataModels.h"
#include "Core/PlayFabClientAPI.h"
#include "PlayFabError.h" 
#include "Planet_ProTypes.h" // [추가] 커스터마이징 구조체 인식을 위해 필수
#include "MyGameInstance.generated.h"

// [1] 저장 완료 알림용 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayFabSaveComplete);

// [추가] 통합 로드 완료 알림용 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayFabLoadComplete);

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
class PLANET_PRO_API UMyGameInstance : public UAdvancedFriendsGameInstance
{
    GENERATED_BODY()

public:
    // ==========================================================
    // 1. 인벤토리 시스템 (기존 유지)
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
    // 2. 시간 저장/로드 시스템 (기존 유지)
    // ==========================================================

    UPROPERTY(BlueprintReadOnly, Category = "TimeSystem")
    float SavedSkyTime = -1.0f; 

    UFUNCTION(BlueprintCallable, Category = "TimeSystem")
    void SaveSkyTime(float CurrentTime);

    UFUNCTION(BlueprintCallable, Category = "TimeSystem")
    void LoadSkyTime();

    UPROPERTY(BlueprintAssignable, Category = "PlayFab")
    FOnSkyTimeLoaded OnSkyTimeLoaded;
    
    UPROPERTY(BlueprintAssignable, Category = "PlayFab")
    FOnPlayFabSaveComplete OnSaveSuccess;

    // ==========================================================
    // 3. [추가됨] 커스터마이징 & 통합 저장 시스템
    // ==========================================================

    // [데이터] 커스터마이징 정보
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
    FCharacterCustomizationData MyCustomData;

    // [리소스] 텍스처 리스트 (블루프린트에서 채워넣기)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Customization Resources")
    TArray<UTexture2D*> BodyTextureList;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Customization Resources")
    TArray<UTexture2D*> EyeTextureList;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Customization Resources")
    TArray<UTexture2D*> MouthTextureList;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Customization Resources")
    TArray<UTexture2D*> ShipTextureList;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Customization Resources")
    TArray<UTexture2D*> SofaTextureList;

    // [함수] 통합 저장 (Inventory + SkyTime + CustomData 모두 저장)
    UFUNCTION(BlueprintCallable, Category = "PlayFab")
    void SaveAllData();

    // [함수] 통합 로그인 및 로드
    UFUNCTION(BlueprintCallable, Category = "PlayFab")
    void LoginAndLoadData();

    // [델리게이트] 통합 로드 완료 신호
    UPROPERTY(BlueprintAssignable, Category = "PlayFab")
    FOnPlayFabLoadComplete OnDataLoadSuccess;

private:
    // 기존 콜백 함수들
    void OnSaveTimeSuccess(const PlayFab::ClientModels::FUpdateUserDataResult& Result);
    void OnLoadTimeSuccess(const PlayFab::ClientModels::FGetUserDataResult& Result);
    void OnTimeError(const PlayFab::FPlayFabCppError& ErrorResult);
    void OnUpdateUserDataSuccess(const PlayFab::ClientModels::FUpdateUserDataResult& Result);
    void OnUpdateUserDataError(const PlayFab::FPlayFabCppError& ErrorResult);

    // [추가됨] 통합 시스템용 콜백 및 헬퍼
    void OnLoginSuccess(const PlayFab::ClientModels::FLoginResult& Result);
    void OnLoginFailure(const PlayFab::FPlayFabCppError& ErrorResult);
    void OnLoadDataSuccess(const PlayFab::ClientModels::FGetUserDataResult& Result); // 통합 로드 성공
    void OnSaveDataSuccess(const PlayFab::ClientModels::FUpdateUserDataResult& Result); // 통합 저장 성공

    // JSON 변환 헬퍼
    FString CustomDataToJson();
    void JsonToInventory(const FString& JsonString); // 로드할 때 필요
    void JsonToCustomData(const FString& JsonString);
};