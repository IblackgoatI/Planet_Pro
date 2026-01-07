// MainHUDWidget.h

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/UniformGridPanel.h"
#include "Components/HorizontalBox.h" // [필수] 호리젠탈 박스 헤더 추가
#include "Planet_ProTypes.h"
#include "MainHUDWidget.generated.h"

UCLASS()
class PLANET_PRO_API UMainHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	class UWidget* InventoryWindow;

	UPROPERTY(meta = (BindWidget))
	class UUniformGridPanel* InventoryGrid;

	// [추가] 퀵슬롯 바 (블루프린트의 Horizontal Box와 연결됨)
	UPROPERTY(meta = (BindWidget))
	class UHorizontalBox* QuickSlotBar; 

	bool bIsInventoryOpen = false;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ToggleInventory();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RefreshInventory(const TArray<FPlanetItemInfo>& InventoryItems);
};