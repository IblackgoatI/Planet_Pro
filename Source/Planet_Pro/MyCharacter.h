#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Planet_ProTypes.h" // 구조체 사용을 위해 필수
// #include "MainHUDWidget.h" // 순환 참조 방지를 위해 여기서는 주석 처리 권장 (cpp에서 포함)

// PlayFab 헤더
#include "PlayFab.h"
#include "PlayFabError.h"
#include "Core/PlayFabClientDataModels.h"
#include "Core/PlayFabClientAPI.h"

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

	// ==========================================================
	// 1. 컴포넌트 & 리소스 (커스터마이징용 변수 선언)
	// ==========================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	class UStaticMeshComponent* WeaponMeshComp; // 무기

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	class USkeletalMeshComponent* Comp_SpaceShip_Skel; // 우주선 (Skeletal)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	class UStaticMeshComponent* Comp_CharBody; // 캐릭터 몸통 (Static)

	// 색상 변경용 머티리얼 인스턴스 (동적 재질)
	UPROPERTY()
	UMaterialInstanceDynamic* DMI_Body;       // 몸통 색
	UPROPERTY()
	UMaterialInstanceDynamic* DMI_Ship_Shell; // 우주선 겉면
	UPROPERTY()
	UMaterialInstanceDynamic* DMI_Ship_Sofa;  // 우주선 소파

	// ==========================================================
	// 2. 인벤토리 & UI
	// ==========================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<FPlanetItemInfo> Inventory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUserWidget> MainHUDClass;

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	class UMainHUDWidget* MainHUDInstance;

	// 인벤토리 조작 함수
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void AddInventoryItem(FName NewItemID, int32 NewAmount);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SwapInventoryItems(int32 SourceIndex, int32 DestinationIndex);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void OnInventoryKeyPressed();

	// ==========================================================
	// 3. PlayFab 저장 / 로드 (cpp에 구현된 함수들 선언)
	// ==========================================================
	void SaveInventoryToPlayFab();
	void OnSaveSuccess(const PlayFab::ClientModels::FUpdateUserDataResult& Result);
	void OnSaveError(const PlayFab::FPlayFabCppError& ErrorResult);

	void LoadInventoryFromPlayFab();
	void OnLoadSuccess(const PlayFab::ClientModels::FGetUserDataResult& Result);
	void OnLoadError(const PlayFab::FPlayFabCppError& ErrorResult);

	// ==========================================================
	// 4. 커스터마이징 적용 함수 (GameInstance -> Character)
	// ==========================================================
	// cpp의 ApplyCustomizationFromGI()를 사용하기 위해 선언
	UFUNCTION(BlueprintCallable, Category = "Customization")
	void ApplyCustomizationFromGI();

	// ==========================================================
	// 5. 퀵슬롯 & 무기 조작
	// ==========================================================
	int32 CurrentSelectedSlotIndex = 0;

	void SelectQuickSlot(int32 SlotIndex);
	void UpdateWeaponVisuals();
	void GetAxeCheat();

	// 키 바인딩용
	void OnQuickSlot1();
	void OnQuickSlot2();
	void OnQuickSlot3();
	void OnQuickSlot4();
	void OnQuickSlot5();
	void OnQuickSlot6();
	void OnQuickSlot7();
	void OnQuickSlot8();
};