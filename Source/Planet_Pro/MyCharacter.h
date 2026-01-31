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

UENUM(BlueprintType)
enum class ECharacterWeaponState : uint8
{
	Unarmed     UMETA(DisplayName = "빈손"),
	Axe         UMETA(DisplayName = "도끼 들음"),
	Wood        UMETA(DisplayName = "통나무 들음"),
	Berry       UMETA(DisplayName = "열매 들음"),
	Syringe     UMETA(DisplayName = "주사기 들음")
};

UCLASS()
class PLANET_PRO_API AMyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMyCharacter();
	// ★ [추가 1] 레플리케이션 필수 함수 (이거 없으면 에러남)
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	// [추가] 저장을 지연시키기 위한 타이머
	FTimerHandle SaveTimerHandle;

public:	
	
	UFUNCTION(Server, Reliable)
	void Server_EquipItem(FName ItemID);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Visuals")
	void BP_UpdateEquippedItem(ECharacterWeaponState NewState);
	
	// ★ [수정 2] 기존 UPROPERTY를 이렇게 고치세요!
	// (ReplicatedUsing = 함수이름) -> 변수 바뀌면 저 함수 자동 실행해라!
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_CurrentWeaponState, Category = "Animation")
	ECharacterWeaponState CurrentWeaponState;
	
	UFUNCTION()
	void OnRep_CurrentWeaponState();
	
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// [추가] 인벤토리에서 아이템 소모 (성공 시 true 반환)
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool ConsumeInventoryItem(FName TargetItemID, int32 AmountToConsume);
	
	// [추가] 즉시 저장하지 않고, 3초 뒤에 몰아서 저장하는 함수
	void RequestSmartSave();
	
	// "지금 들고 있는 아이템이 도끼인지?
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsAxeEquipped();
	
	// ==========================================================
	// 1. 컴포넌트 & 리소스 (커스터마이징용 변수 선언)
	// ==========================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	class UStaticMeshComponent* WeaponMeshComp; // 무기

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	class USkeletalMeshComponent* Comp_SpaceShip_Skel; // 우주선 (Skeletal)
	
	// ★ [추가] 커마창 우주선 (Static) - BP_Customizing용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	class UStaticMeshComponent* Comp_SpaceShip_Static;

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
	
	// [추가] 아이템 교환 함수 (재료 이름, 재료 개수, 보상 이름, 보상 개수)
	UFUNCTION(BlueprintCallable, Category = "Store")
	bool TryExchangeItem(FName CostItemID, int32 CostAmount, FName RewardItemID, int32 RewardAmount);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	class USkeletalMeshComponent* Comp_EquippedSyringe;
	
	UFUNCTION(BlueprintCallable, Category = "Store")
	bool BuyAxe(); // 베리 10개 -> 도끼

	UFUNCTION(BlueprintCallable, Category = "Store")
	bool BuySyringe(); // 나무 15개 + 베리 10개 -> 주사기
    
	// 재료 확인용 헬퍼 함수
	bool HasItem(FName ItemID, int32 Amount);
};