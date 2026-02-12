#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Planet_ProTypes.generated.h"

USTRUCT(BlueprintType)
struct FCharacterCustomizationData
{
	GENERATED_BODY()

public:
	// 기본값 0으로 초기화 (안 하면 쓰레기값 들어갈 수 있음)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Customization")
	int32 BodyIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Customization")
	int32 EyeIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Customization")
	int32 MouthIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Customization")
	int32 MachineIndex = 0;
};

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