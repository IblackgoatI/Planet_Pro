#include "ItemSlotWidget.h"
#include "InventoryDragDropOperation.h" // 1단계에서 만든 헤더
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "MyCharacter.h" // 캐릭터 함수 호출용
#include "Components/Image.h" // [필수] 이미지 컴포넌트 헤더


// [중요] 클래스 이름은 UItemSlotWidget 이어야 합니다!
void UItemSlotWidget::UpdateSlot(FName NewItemID, int32 NewAmount)
{
	// [추가] 나중을 위해 정보 저장 (백업)
	this->SavedItemID = NewItemID;
	this->SavedAmount = NewAmount;
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
// =============================================================
// [수정 1] 빈 슬롯 클릭 방지 (마우스 누르는 순간 검사)
// =============================================================
FReply UItemSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 좌클릭이 아니면 무시
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	// [핵심] 아이콘이 숨겨져 있다 = 빈 슬롯이다 -> 드래그 금지!
	// (UpdateSlot 함수에서 아이템 없으면 Hidden으로 설정했으므로 이걸로 판단 가능)
	if (ItemIcon && ItemIcon->GetVisibility() == ESlateVisibility::Hidden)
	{
		return FReply::Unhandled(); // "나 건드리지 마" 하고 시스템에 반환
	}

	// 아이템이 있을 때만 드래그 감지 시작
	return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
}

// 2. 드래그가 진짜로 감지됐을 때 -> "트럭(Operation) 출발 시킴"
// =============================================================
// [수정 2] 드래그 위치 보정 (Offset 사용)
// =============================================================
void UItemSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	UInventoryDragDropOperation* DragOp = NewObject<UInventoryDragDropOperation>();
	DragOp->SourceIndex = this->MyIndex;

	// =============================================================
	// [핵심 수정] 나 자신(this) 대신, 새로운 "복제 위젯"을 만듭니다.
	// =============================================================
    
	// 1. 나랑 똑같은 클래스로 위젯 하나 생성 (가짜 아이콘)
	if (GetOwningPlayer())
	{
		UItemSlotWidget* DragVisual = CreateWidget<UItemSlotWidget>(GetOwningPlayer(), GetClass());

		if (DragVisual)
		{
			// 2. 가짜 위젯한테 "너는 아까 저장한 그 아이템이야"라고 알려줌 (그래야 아이콘이 뜸)
			DragVisual->UpdateSlot(this->SavedItemID, this->SavedAmount);

			// 3. [중요] 사이즈 강제 지정 (안 하면 0x0 돼서 안 보일 수 있음)
			// 본인 슬롯 크기에 맞춰서 조절하세요 (예: 80x80)
			DragVisual->SetDesiredSizeInViewport(FVector2D(80.0f, 80.0f));

			// 4. 드래그 비주얼로 설정
			DragOp->DefaultDragVisual = DragVisual;
		}
		// [추가] 드래그가 시작됐으니, "원본(나)"은 잠시 투명인간 만들기!
		if (ItemIcon) ItemIcon->SetVisibility(ESlateVisibility::Hidden);
		if (ItemAmountText) ItemAmountText->SetVisibility(ESlateVisibility::Hidden);
	}

	// 위치 보정 (중앙 정렬 + 살짝 오프셋)
	DragOp->Pivot = EDragPivot::MouseDown; 
	DragOp->Offset = FVector2D(0.1f, 0.1f); // 슬롯 크기의 절반만큼 빼주기

	OutOperation = DragOp;
}

void UItemSlotWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragCancelled(InDragDropEvent, InOperation);

	// 드래그가 취소됐으므로, 숨겼던 아이콘을 다시 원상복구!
	// (RefreshInventory가 호출되지 않는 상황을 대비한 안전장치)
	if (this->SavedAmount > 0)
	{
		if (ItemIcon) ItemIcon->SetVisibility(ESlateVisibility::Visible); // 혹은 NotHitTestable
		if (ItemAmountText) ItemAmountText->SetVisibility(ESlateVisibility::Visible);
	}
}

bool UItemSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);

	UE_LOG(LogTemp, Warning, TEXT("📦 [1] 드롭 감지됨! 도착 슬롯 번호(MyIndex): %d"), this->MyIndex);

	// 1. 가져온 트럭(Operation)이 내 트럭이 맞는지 확인
	UInventoryDragDropOperation* InventoryOp = Cast<UInventoryDragDropOperation>(InOperation);
	if (!InventoryOp)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [2] 트럭 종류가 다름! (Cast 실패)"));
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("📦 [2] 트럭 확인됨. 출발 슬롯 번호(SourceIndex): %d"), InventoryOp->SourceIndex);

	// 2. 출발지와 도착지가 같은지 확인 (제자리 걸음)
	if (InventoryOp->SourceIndex == this->MyIndex)
	{
		UE_LOG(LogTemp, Warning, TEXT("⚠️ [3] 제자리 드롭입니다. (무시함)"));
		return false;
	}

	// 3. 캐릭터(Pawn) 찾기
	AMyCharacter* MyChar = Cast<AMyCharacter>(GetOwningPlayerPawn());
	if (!MyChar)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ [4] 캐릭터(MyCharacter)를 못 찾음! Cast 실패."));
		return false;
	}

	// 4. 교환 요청
	UE_LOG(LogTemp, Warning, TEXT("✅ [5] 교환 시도! %d번 <-> %d번"), InventoryOp->SourceIndex, this->MyIndex);
	MyChar->SwapInventoryItems(InventoryOp->SourceIndex, this->MyIndex);
    
	return true;
}

void UItemSlotWidget::SetIsSelected(bool bSelected)
{
	// 방어 코드: 테두리 이미지가 없으면 아무것도 안 함
	if (!Img_Outline) return;

	if (bSelected)
	{
		// 선택됨: 테두리 보이기! (Visible)
		Img_Outline->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		// 선택 해제: 테두리 숨기기! (Hidden)
		Img_Outline->SetVisibility(ESlateVisibility::Hidden);
	}
}

