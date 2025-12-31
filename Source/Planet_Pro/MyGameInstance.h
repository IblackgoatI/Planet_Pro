#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"

// [중요] 외부 플러그인(PlayFab) 헤더는 무조건 .generated.h 보다 위에 있어야 합니다!
#include "PlayFab.h"
#include "Core/PlayFabClientDataModels.h"
#include "Core/PlayFabClientAPI.h"

// [필수] 이 generated 헤더는 항상 include 목록 중 '맨 마지막'이어야 합니다.
#include "MyGameInstance.generated.h"

// 1. 아이템 데이터 구조체
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

// 2. 클래스 정의
UCLASS()
class PLANET_PRO_API UMyGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// 인벤토리 배열
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<FItemData> MyInventory;

	// --- 기존 함수들 ---
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddOrUpdateItem(FName InItemID, int32 InAmount);
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	FString GetInventoryAsJsonString();
	
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	TMap<FString, FString> GetInventoryMapForPlayFab();
	
	// --- [NEW] 대망의 자동 저장 함수 ---
	
	// 이 함수 하나만 부르면 저장 끝 (핀 연결 X)
	UFUNCTION(BlueprintCallable, Category = "PlayFab")
	void SaveInventoryToPlayFab_CPP();
};