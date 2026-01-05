#include "ItemSlotWidget.h"

// [중요] 클래스 이름은 UItemSlotWidget 이어야 합니다!
void UItemSlotWidget::UpdateSlot(FName NewItemID, int32 NewAmount)
{
	// 1. 수량이 0 이하거나 없다? -> 숨기기
	if (NewAmount <= 0)
	{
		if (ItemIcon) 
		{
			ItemIcon->SetVisibility(ESlateVisibility::Hidden);
		}
		
		if (ItemAmountText) 
		{
			ItemAmountText->SetVisibility(ESlateVisibility::Hidden); // 텍스트도 숨김
			ItemAmountText->SetText(FText::GetEmpty());
		}
	}
	// 2. 아이템이 있다! -> 보이기 & 업데이트
	else
	{
		// 2-1. 아이콘 처리
		if (ItemIcon)
		{
			ItemIcon->SetVisibility(ESlateVisibility::Visible);
			
			// [핵심] 블루프린트야, 이 ID("Wood")에 맞는 그림으로 바꿔줘!
			SetIconFromItemID(NewItemID); 
		}

		// 2-2. 수량 텍스트 처리
		if (ItemAmountText)
		{
			ItemAmountText->SetVisibility(ESlateVisibility::Visible);
			ItemAmountText->SetText(FText::AsNumber(NewAmount));
		}
	}
}