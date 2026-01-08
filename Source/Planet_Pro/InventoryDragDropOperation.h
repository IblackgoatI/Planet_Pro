#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "InventoryDragDropOperation.generated.h"

/**
 * 드래그 하는 동안 "데이터(원래 위치)"를 배달하는 트럭
 */
UCLASS()
class PLANET_PRO_API UInventoryDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	// 출발한 슬롯 번호 (몇 번 칸에서 왔니?)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SourceIndex;
};