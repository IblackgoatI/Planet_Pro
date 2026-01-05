#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Planet_ProTypes.generated.h"

// [인벤토리용] 내 가방 속 아이템 정보 (이름 변경됨!)
USTRUCT(BlueprintType)
struct FPlanetItemInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Amount;
};

// [데이터테이블용] 아이템 상세 정보
USTRUCT(BlueprintType)
struct FItemTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UTexture2D* Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxStack;
};