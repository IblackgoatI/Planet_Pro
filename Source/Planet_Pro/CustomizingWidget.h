#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "CustomizingWidget.generated.h"

// 전방 선언
class APreviewCharacter;
class UMyGameInstance;

UCLASS()
class PLANET_PRO_API UCustomizingWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;

public:
    // =================================================================
    // [1] 버튼 바인딩 (이름을 BP와 똑같이 맞춰야 함!)
    // =================================================================
    
    // 몸통 버튼 (3개)
    UPROPERTY(meta = (BindWidget)) UButton* Btn_Body_0;
    UPROPERTY(meta = (BindWidget)) UButton* Btn_Body_1;
    UPROPERTY(meta = (BindWidget)) UButton* Btn_Body_2;

    // 우주선 버튼 (5개)
    UPROPERTY(meta = (BindWidget)) UButton* Btn_Ship_0;
    UPROPERTY(meta = (BindWidget)) UButton* Btn_Ship_1;
    UPROPERTY(meta = (BindWidget)) UButton* Btn_Ship_2;
    UPROPERTY(meta = (BindWidget)) UButton* Btn_Ship_3;
    UPROPERTY(meta = (BindWidget)) UButton* Btn_Ship_4;

    // ★ [추가] 눈 버튼 (5개)
    UPROPERTY(meta = (BindWidget)) UButton* Btn_Eye_0;
    UPROPERTY(meta = (BindWidget)) UButton* Btn_Eye_1;
    UPROPERTY(meta = (BindWidget)) UButton* Btn_Eye_2;
    UPROPERTY(meta = (BindWidget)) UButton* Btn_Eye_3;
    UPROPERTY(meta = (BindWidget)) UButton* Btn_Eye_4;

    // ★ [추가] 입 버튼 (5개)
    UPROPERTY(meta = (BindWidget)) UButton* Btn_Mouth_0;
    UPROPERTY(meta = (BindWidget)) UButton* Btn_Mouth_1;
    UPROPERTY(meta = (BindWidget)) UButton* Btn_Mouth_2;
    UPROPERTY(meta = (BindWidget)) UButton* Btn_Mouth_3;
    UPROPERTY(meta = (BindWidget)) UButton* Btn_Mouth_4;

    // 저장 버튼
    UPROPERTY(meta = (BindWidget)) UButton* Btn_Save;

    // =================================================================
    // [2] 참조 변수
    // =================================================================
    UPROPERTY(BlueprintReadWrite, Category = "Ref")
    APreviewCharacter* Ref_PreviewChar;

    UPROPERTY(BlueprintReadWrite, Category = "Ref")
    UMyGameInstance* Ref_GameInstance;

private:
    // =================================================================
    // [3] 로직 함수
    // =================================================================
    
    // 공통 처리 (저장 + 프리뷰 갱신)
    void SetBodyIndex(int32 Index);
    void SetShipIndex(int32 Index);
    void SetEyeIndex(int32 Index);   // 추가
    void SetMouthIndex(int32 Index); // 추가

    // 클릭 핸들러 - 몸통
    UFUNCTION() void OnClick_Body_0();
    UFUNCTION() void OnClick_Body_1();
    UFUNCTION() void OnClick_Body_2();

    // 클릭 핸들러 - 우주선
    UFUNCTION() void OnClick_Ship_0();
    UFUNCTION() void OnClick_Ship_1();
    UFUNCTION() void OnClick_Ship_2();
    UFUNCTION() void OnClick_Ship_3();
    UFUNCTION() void OnClick_Ship_4();

    // ★ [추가] 클릭 핸들러 - 눈
    UFUNCTION() void OnClick_Eye_0();
    UFUNCTION() void OnClick_Eye_1();
    UFUNCTION() void OnClick_Eye_2();
    UFUNCTION() void OnClick_Eye_3();
    UFUNCTION() void OnClick_Eye_4();

    // ★ [추가] 클릭 핸들러 - 입
    UFUNCTION() void OnClick_Mouth_0();
    UFUNCTION() void OnClick_Mouth_1();
    UFUNCTION() void OnClick_Mouth_2();
    UFUNCTION() void OnClick_Mouth_3();
    UFUNCTION() void OnClick_Mouth_4();

    UFUNCTION() void OnClick_Save();
};