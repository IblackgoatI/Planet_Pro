#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/UniformGridPanel.h" // 그리드 패널 사용을 위해 필수
#include "Planet_ProTypes.h"           // 구조체(FPlanetItemInfo) 사용을 위해 필수
#include "MainHUDWidget.generated.h"

UCLASS()
class PLANET_PRO_API UMainHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// [기존] 인벤토리 창 전체 (배경 포함)
	UPROPERTY(meta = (BindWidget))
	class UWidget* InventoryWindow;

	// [추가] 아이템 슬롯들이 정렬될 격자판 (UniformGridPanel)
	// ※ WBP_MainHUD 안의 GridPanel 이름도 "InventoryGrid"로 맞춰야 함!
	UPROPERTY(meta = (BindWidget))
	class UUniformGridPanel* InventoryGrid;

	// 인벤토리 열림 상태 체크
	bool bIsInventoryOpen = false;

	// [기존] 열고 닫기 함수
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ToggleInventory();

	// [추가] 아이템 데이터 받아서 화면 새로고침하는 함수
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RefreshInventory(const TArray<FPlanetItemInfo>& InventoryItems);
};