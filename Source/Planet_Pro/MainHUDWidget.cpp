#include "MainHUDWidget.h"
#include "ItemSlotWidget.h" // [필수] 슬롯 위젯을 제어하기 위해 헤더 포함

void UMainHUDWidget::ToggleInventory()
{
	// 방어 코드: 인벤토리 창이 없으면 아무것도 안 함
	if (!InventoryWindow) return;

	// 상태 뒤집기 (닫힘 <-> 열림)
	bIsInventoryOpen = !bIsInventoryOpen;
	
	// 플레이어 컨트롤러 가져오기 (마우스 조종용)
	APlayerController* PC = GetOwningPlayer();

	if (bIsInventoryOpen)
	{
		// 1. 열기 (보이게)
		InventoryWindow->SetVisibility(ESlateVisibility::Visible);
		
		// 2. 마우스 켜고 UI 모드로 전환
		if (PC)
		{
			PC->bShowMouseCursor = true;
			PC->SetInputMode(FInputModeGameAndUI());
		}
	}
	else
	{
		// 1. 닫기 (숨기기)
		InventoryWindow->SetVisibility(ESlateVisibility::Hidden);

		// 2. 마우스 끄고 게임 모드로 복귀
		if (PC)
		{
			PC->bShowMouseCursor = false;
			PC->SetInputMode(FInputModeGameOnly());
		}
	}
}

void UMainHUDWidget::RefreshInventory(const TArray<FPlanetItemInfo>& InventoryItems)
{
	// 방어 코드: 그리드 패널이 연결 안 됐으면 중단
	if (!InventoryGrid) return;

	// 1. 그리드 패널 안의 모든 자식(슬롯 위젯들)을 가져옴
	TArray<UWidget*> Slots = InventoryGrid->GetAllChildren();

	// 2. 슬롯 개수만큼 반복
	for (int32 i = 0; i < Slots.Num(); i++)
	{
		// 자식 위젯을 ItemSlotWidget으로 형변환 (Cast)
		UItemSlotWidget* SlotWidget = Cast<UItemSlotWidget>(Slots[i]);

		// 형변환 성공 시 (즉, 올바른 슬롯 위젯이라면)
		if (SlotWidget)
		{
			// 내 인벤토리 데이터(배열)에 i번째 아이템이 존재하는지 확인
			if (InventoryItems.IsValidIndex(i))
			{
				// 데이터가 있음 -> 아이템 정보로 슬롯 업데이트
				SlotWidget->UpdateSlot(InventoryItems[i].ItemID, InventoryItems[i].Amount);
			}
			else
			{
				// 데이터가 없음 -> 빈칸으로 초기화 (None, 0개)
				SlotWidget->UpdateSlot(FName("None"), 0);
			}
		}
	}
}