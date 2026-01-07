// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"      // 이미지 컴포넌트
#include "Components/TextBlock.h"  // 텍스트 컴포넌트
#include "ItemSlotWidget.generated.h"


/**
 * 인벤토리 슬롯 하나를 담당하는 위젯 클래스
 */
UCLASS()
class PLANET_PRO_API UItemSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	
	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	int32 MyIndex;

	// [추가] 내가 무슨 아이템인지 기억할 변수
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FName SavedItemID;

	// [추가] 몇 개인지 기억할 변수
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 SavedAmount;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
	class UImage* Img_BG;
	
	// [추가] 선택 여부에 따라 스타일을 바꾸는 함수
	void SetIsSelected(bool bSelected);
	
	// 1. 아이콘 이미지 (BlueprintReadWrite 추가 -> 그래프에서 변수로 보임!)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
	class UImage* ItemIcon;

	// 2. 수량 텍스트 (BlueprintReadWrite 추가 -> 그래프에서 변수로 보임!)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
	class UTextBlock* ItemAmountText;

	// 3. 슬롯 데이터 업데이트 함수 (CPP 로직)
	UFUNCTION(BlueprintCallable)
	void UpdateSlot(FName NewItemID, int32 NewAmount);

	// 4. 아이콘 변경 이벤트 (BP에서 구현)
	UFUNCTION(BlueprintImplementableEvent)
	void SetIconFromItemID(FName ItemID);

	
protected:
	// 1. 마우스 클릭 감지 (드래그 시작 준비)
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// 2. 드래그 시작 (실제 드래그 발생)
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;

	// 3. 드롭 감지 (도착)
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	
	// [추가] 드래그가 취소됐을 때(허공에 놓았을 때) 호출되는 함수
	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
};