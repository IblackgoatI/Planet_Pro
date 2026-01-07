#include "MainHUDWidget.h"
#include "ItemSlotWidget.h"
#include "Components/HorizontalBox.h" 
#include "Components/UniformGridSlot.h" // [필수] 행/열 계산을 위해 필요

void UMainHUDWidget::ToggleInventory()
{
    if (!InventoryWindow) return;

    bIsInventoryOpen = !bIsInventoryOpen;
    
    APlayerController* PC = GetOwningPlayer();

    if (bIsInventoryOpen)
    {
        InventoryWindow->SetVisibility(ESlateVisibility::Visible);
        if (PC)
        {
            PC->bShowMouseCursor = true;
            PC->SetInputMode(FInputModeGameAndUI());
        }
    }
    else
    {
        InventoryWindow->SetVisibility(ESlateVisibility::Hidden);
        if (PC)
        {
            PC->bShowMouseCursor = false;
            PC->SetInputMode(FInputModeGameOnly());
        }
    }
}

void UMainHUDWidget::RefreshInventory(const TArray<FPlanetItemInfo>& InventoryItems)
{
    // =========================================================
    // 1. 퀵슬롯 업데이트 (가로 1줄, 0 ~ 9번)
    // =========================================================
    if (QuickSlotBar)
    {
        TArray<UWidget*> QSlots = QuickSlotBar->GetAllChildren();
        
        for (int32 i = 0; i < QSlots.Num(); i++)
        {
            UItemSlotWidget* SlotWidget = Cast<UItemSlotWidget>(QSlots[i]);
            if (SlotWidget)
            {
                SlotWidget->MyIndex = i; // 퀵슬롯 번호 (0~9)

                if (InventoryItems.IsValidIndex(i))
                {
                    SlotWidget->UpdateSlot(InventoryItems[i].ItemID, InventoryItems[i].Amount);
                }
                else
                {
                    SlotWidget->UpdateSlot(FName("None"), 0);
                }
            }
        }
    }

    // =========================================================
    // 2. 메인 인벤토리 업데이트 (행/열 계산 방식)
    // =========================================================
    if (InventoryGrid)
    {
        TArray<UWidget*> GSlots = InventoryGrid->GetAllChildren();
        
        // 퀵슬롯 개수 (0~9번까지 10개)
        const int32 QuickSlotCount = 10; 
        
        // [중요 체크] 본인 UI의 가로 칸 수에 맞춰주세요! (사진상으로는 6칸, 코드엔 8칸으로 적으셨음)
        // 만약 가로가 6칸이면 6으로, 8칸이면 8로 고치세요.
        const int32 ColumnsPerRow = 8; 

        for (UWidget* Widget : GSlots)
        {
            UItemSlotWidget* SlotWidget = Cast<UItemSlotWidget>(Widget);
            if (SlotWidget)
            {
                // 1. 이 슬롯이 그리드 몇 번째 줄, 몇 번째 칸에 있는지 알아냄
                UUniformGridSlot* GridSlot = Cast<UUniformGridSlot>(SlotWidget->Slot);
                
                int32 FinalIndex = -1;

                if (GridSlot)
                {
                    int32 Row = GridSlot->GetRow();
                    int32 Col = GridSlot->GetColumn();

                    // 2. 수학 공식으로 번호 계산: (줄 번호 * 칸수) + 칸 번호 + 퀵슬롯개수
                    FinalIndex = (Row * ColumnsPerRow) + Col + QuickSlotCount;
                }
                else
                {
                    continue; 
                }

                // 3. 번호표 부여 (드래그 앤 드롭의 핵심!)
                SlotWidget->MyIndex = FinalIndex;

                // 4. 아이템 표시
                if (InventoryItems.IsValidIndex(FinalIndex))
                {
                    SlotWidget->UpdateSlot(InventoryItems[FinalIndex].ItemID, InventoryItems[FinalIndex].Amount);
                }
                else
                {
                    SlotWidget->UpdateSlot(FName("None"), 0);
                }
            }
        }
    }
}