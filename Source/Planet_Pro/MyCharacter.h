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
	
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// [추가] 현재 선택된 퀵슬롯 번호 (0 ~ 7)
	int32 CurrentSelectedSlotIndex = 0;

	// [추가] 키보드 입력 처리 함수들
	void SelectQuickSlot(int32 SlotIndex); // 공통 처리 함수
    
	// 키 바인딩용 함수들
	void OnQuickSlot1();
	void OnQuickSlot2();
	void OnQuickSlot3();
	void OnQuickSlot4();
	void OnQuickSlot5();
	void OnQuickSlot6();
	void OnQuickSlot7();
	void OnQuickSlot8();
	
	// 인벤토리 두 칸의 위치를 바꾸는 함수
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SwapInventoryItems(int32 SourceIndex, int32 DestinationIndex);
	
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
	//void AddTestItem(); 테스트용 코드
	void AddInventoryItem(FName NewItemID, int32 NewAmount);

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
	
	// [추가] 도끼(무기) 메쉬를 제어하기 위한 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	UStaticMeshComponent* WeaponMeshComp;

	// [추가] =키 눌러서 도끼 얻는 치트 함수
	void GetAxeCheat();
    
	// [추가] 퀵슬롯 선택 시 무기 보여줄지 말지 결정하는 함수
	void UpdateWeaponVisuals();
};