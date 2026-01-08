#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MinimapWidget.generated.h"

UCLASS()
class PLANET_PRO_API UMinimapWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeConstruct() override;

protected:
	UPROPERTY(meta = (BindWidget))
	class UImage* Img_MapLayer;

	UPROPERTY(meta = (BindWidget))
	class UImage* Img_PlayerArrow;

	// 기본값 20000 (카메라 Ortho Width랑 똑같아야 함)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	float WorldMapSize = 20000.0f; 

private:
	UPROPERTY()
	class UMaterialInstanceDynamic* MinimapMatInst;
};