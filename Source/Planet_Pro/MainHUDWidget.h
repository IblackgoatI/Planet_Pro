// MainHUDWidget.h

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/UniformGridPanel.h"
#include "Components/HorizontalBox.h" // [필수] 호리젠탈 박스 헤더 추가
#include "Planet_ProTypes.h"
#include "MainHUDWidget.generated.h"

class UTextBlock;
class UImage;
class UTexture2D;

UCLASS()
class PLANET_PRO_API UMainHUDWidget : public UUserWidget
{
	GENERATED_BODY()
	
protected:
	// 위젯 생성 시 1회 실행
	virtual void NativeConstruct() override;
	// 매 프레임 실행 (시간 갱신용)
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	// [UI 바인딩] 에디터의 위젯 이름과 똑같아야 합니다!
	UPROPERTY(meta = (BindWidget))
	UTextBlock* Txt_GameTime;

	UPROPERTY(meta = (BindWidget))
	UImage* Img_SunMoon;

	// [설정] 에디터에서 해/달 이미지를 꽂아주세요
	UPROPERTY(EditDefaultsOnly, Category = "GameTime")
	UTexture2D* Icon_Sun;

	UPROPERTY(EditDefaultsOnly, Category = "GameTime")
	UTexture2D* Icon_Moon;
	
	UPROPERTY(meta = (BindWidget))
	class UWidget* InventoryWindow;

	UPROPERTY(meta = (BindWidget))
	class UUniformGridPanel* InventoryGrid;

	// [추가] 특정 번호의 퀵슬롯만 강조하는 함수
	UFUNCTION(BlueprintCallable)
	void UpdateQuickSlotHighlight(int32 SelectedIndex);
	
	// [추가] 퀵슬롯 바 (블루프린트의 Horizontal Box와 연결됨)
	UPROPERTY(meta = (BindWidget))
	class UHorizontalBox* QuickSlotBar; 

	bool bIsInventoryOpen = false;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ToggleInventory();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RefreshInventory(const TArray<FPlanetItemInfo>& InventoryItems);
	
	// [설정] 게임 하루가 현실 시간으로 몇 분인지? (예: 5.0 = 5분)
	// 이 숫자를 바꾸면 시간 속도가 변합니다!
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameTime")
	float RealMinutesPerDay = 5.0f;
	
private:
	// 하늘 액터를 찾아서 저장해둘 변수
	UPROPERTY()
	AActor* SkyActor;

	// 시간을 업데이트하는 내부 함수
	void UpdateGameTime();
	
	// C++ 내부에서 계산할 진짜 게임 시간 (0 ~ 2400)
	float InternalGameTime = 800.0f; // 기본값: 아침 8시
};