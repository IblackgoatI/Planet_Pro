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
};