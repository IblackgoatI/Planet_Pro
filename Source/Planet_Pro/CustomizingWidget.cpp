#include "CustomizingWidget.h"
#include "PreviewCharacter.h"
#include "MyGameInstance.h"
#include "Kismet/GameplayStatics.h"

void UCustomizingWidget::NativeConstruct()
{
    Super::NativeConstruct();

    UE_LOG(LogTemp, Warning, TEXT("================ [Widget] NativeConstruct 시작 ================"));

    // 1. 프리뷰 캐릭터 찾기
    AActor* FoundActor = UGameplayStatics::GetActorOfClass(GetWorld(), APreviewCharacter::StaticClass());
    Ref_PreviewChar = Cast<APreviewCharacter>(FoundActor);

    if (Ref_PreviewChar)
    {
        UE_LOG(LogTemp, Warning, TEXT("✅ [Widget] PreviewCharacter 찾기 성공!"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ [Widget] PreviewCharacter를 찾을 수 없습니다! (레벨에 배치했나요?)"));
    }

    // 2. 게임 인스턴스 가져오기
    Ref_GameInstance = Cast<UMyGameInstance>(GetGameInstance());
    if (Ref_GameInstance)
    {
        UE_LOG(LogTemp, Warning, TEXT("✅ [Widget] GameInstance 찾기 성공!"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ [Widget] GameInstance가 Null입니다!"));
    }

    // 3. 버튼 연결 확인 (대표로 하나만 체크)
    if (!Btn_Ship_0)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ [Widget] Btn_Ship_0 버튼이 바인딩되지 않았습니다! (이름이 틀렸거나 위젯에 없음)"));
    }

    // 몸통 & 우주선 버튼 연결
    if (Btn_Body_0) Btn_Body_0->OnClicked.AddDynamic(this, &UCustomizingWidget::OnClick_Body_0);
    if (Btn_Body_1) Btn_Body_1->OnClicked.AddDynamic(this, &UCustomizingWidget::OnClick_Body_1);
    if (Btn_Body_2) Btn_Body_2->OnClicked.AddDynamic(this, &UCustomizingWidget::OnClick_Body_2);

    if (Btn_Ship_0) Btn_Ship_0->OnClicked.AddDynamic(this, &UCustomizingWidget::OnClick_Ship_0);
    if (Btn_Ship_1) Btn_Ship_1->OnClicked.AddDynamic(this, &UCustomizingWidget::OnClick_Ship_1);
    if (Btn_Ship_2) Btn_Ship_2->OnClicked.AddDynamic(this, &UCustomizingWidget::OnClick_Ship_2);
    if (Btn_Ship_3) Btn_Ship_3->OnClicked.AddDynamic(this, &UCustomizingWidget::OnClick_Ship_3);
    if (Btn_Ship_4) Btn_Ship_4->OnClicked.AddDynamic(this, &UCustomizingWidget::OnClick_Ship_4);

    // 눈 버튼 연결
    if (Btn_Eye_0) Btn_Eye_0->OnClicked.AddDynamic(this, &UCustomizingWidget::OnClick_Eye_0);
    if (Btn_Eye_1) Btn_Eye_1->OnClicked.AddDynamic(this, &UCustomizingWidget::OnClick_Eye_1);
    if (Btn_Eye_2) Btn_Eye_2->OnClicked.AddDynamic(this, &UCustomizingWidget::OnClick_Eye_2);
    if (Btn_Eye_3) Btn_Eye_3->OnClicked.AddDynamic(this, &UCustomizingWidget::OnClick_Eye_3);
    if (Btn_Eye_4) Btn_Eye_4->OnClicked.AddDynamic(this, &UCustomizingWidget::OnClick_Eye_4);

    // 입 버튼 연결
    if (Btn_Mouth_0) Btn_Mouth_0->OnClicked.AddDynamic(this, &UCustomizingWidget::OnClick_Mouth_0);
    if (Btn_Mouth_1) Btn_Mouth_1->OnClicked.AddDynamic(this, &UCustomizingWidget::OnClick_Mouth_1);
    if (Btn_Mouth_2) Btn_Mouth_2->OnClicked.AddDynamic(this, &UCustomizingWidget::OnClick_Mouth_2);
    if (Btn_Mouth_3) Btn_Mouth_3->OnClicked.AddDynamic(this, &UCustomizingWidget::OnClick_Mouth_3);
    if (Btn_Mouth_4) Btn_Mouth_4->OnClicked.AddDynamic(this, &UCustomizingWidget::OnClick_Mouth_4);

    // 저장 버튼
    if (Btn_Save) Btn_Save->OnClicked.AddDynamic(this, &UCustomizingWidget::OnClick_Save);
}

// =================================================================
// 헬퍼 함수 (로그 추가됨)
// =================================================================

void UCustomizingWidget::SetBodyIndex(int32 Index)
{
    UE_LOG(LogTemp, Warning, TEXT("🖱️ [Click] 몸통(Body) 버튼 %d번 클릭됨"), Index);

    if (Ref_GameInstance)
    {
        Ref_GameInstance->MyCustomData.BodyIndex = Index;
        if (Ref_PreviewChar)
        {
            int32 Eye = Ref_GameInstance->MyCustomData.EyeIndex;
            int32 Mouth = Ref_GameInstance->MyCustomData.MouthIndex;
            Ref_PreviewChar->UpdateParts(Index, Eye, Mouth);
            UE_LOG(LogTemp, Warning, TEXT("📡 [Send] PreviewChar->UpdateParts 호출 완료 (Body: %d)"), Index);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("❌ PreviewChar가 없어서 몸통 변경 명령 실패"));
        }
    }
}

void UCustomizingWidget::SetEyeIndex(int32 Index)
{
    UE_LOG(LogTemp, Warning, TEXT("🖱️ [Click] 눈(Eye) 버튼 %d번 클릭됨"), Index);

    if (Ref_GameInstance)
    {
        Ref_GameInstance->MyCustomData.EyeIndex = Index;
        if (Ref_PreviewChar)
        {
            int32 Body = Ref_GameInstance->MyCustomData.BodyIndex;
            int32 Mouth = Ref_GameInstance->MyCustomData.MouthIndex;
            Ref_PreviewChar->UpdateParts(Body, Index, Mouth);
            UE_LOG(LogTemp, Warning, TEXT("📡 [Send] PreviewChar->UpdateParts 호출 완료 (Eye: %d)"), Index);
        }
        else
        {
             UE_LOG(LogTemp, Error, TEXT("❌ PreviewChar가 없어서 눈 변경 명령 실패"));
        }
    }
}

void UCustomizingWidget::SetMouthIndex(int32 Index)
{
    UE_LOG(LogTemp, Warning, TEXT("🖱️ [Click] 입(Mouth) 버튼 %d번 클릭됨"), Index);

    if (Ref_GameInstance)
    {
        Ref_GameInstance->MyCustomData.MouthIndex = Index;
        if (Ref_PreviewChar)
        {
            int32 Body = Ref_GameInstance->MyCustomData.BodyIndex;
            int32 Eye = Ref_GameInstance->MyCustomData.EyeIndex;
            Ref_PreviewChar->UpdateParts(Body, Eye, Index);
            UE_LOG(LogTemp, Warning, TEXT("📡 [Send] PreviewChar->UpdateParts 호출 완료 (Mouth: %d)"), Index);
        }
        else
        {
             UE_LOG(LogTemp, Error, TEXT("❌ PreviewChar가 없어서 입 변경 명령 실패"));
        }
    }
}

void UCustomizingWidget::SetShipIndex(int32 Index)
{
    UE_LOG(LogTemp, Warning, TEXT("🖱️ [Click] 우주선(Ship) 버튼 %d번 클릭됨"), Index);

    if (Ref_GameInstance)
    {
        Ref_GameInstance->MyCustomData.MachineIndex = Index;
        if (Ref_PreviewChar) 
        {
            Ref_PreviewChar->UpdateMachine(Index);
            UE_LOG(LogTemp, Warning, TEXT("📡 [Send] PreviewChar->UpdateMachine(%d) 호출 완료"), Index);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("❌ PreviewChar가 없어서 우주선 변경 명령 실패"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ GameInstance가 없어서 데이터 저장이 안 됨"));
    }
}

// =================================================================
// 클릭 핸들러
// =================================================================

void UCustomizingWidget::OnClick_Body_0() { SetBodyIndex(0); }
void UCustomizingWidget::OnClick_Body_1() { SetBodyIndex(1); }
void UCustomizingWidget::OnClick_Body_2() { SetBodyIndex(2); }

void UCustomizingWidget::OnClick_Ship_0() { SetShipIndex(0); }
void UCustomizingWidget::OnClick_Ship_1() { SetShipIndex(1); }
void UCustomizingWidget::OnClick_Ship_2() { SetShipIndex(2); }
void UCustomizingWidget::OnClick_Ship_3() { SetShipIndex(3); }
void UCustomizingWidget::OnClick_Ship_4() { SetShipIndex(4); }

void UCustomizingWidget::OnClick_Eye_0() { SetEyeIndex(0); }
void UCustomizingWidget::OnClick_Eye_1() { SetEyeIndex(1); }
void UCustomizingWidget::OnClick_Eye_2() { SetEyeIndex(2); }
void UCustomizingWidget::OnClick_Eye_3() { SetEyeIndex(3); }
void UCustomizingWidget::OnClick_Eye_4() { SetEyeIndex(4); }

void UCustomizingWidget::OnClick_Mouth_0() { SetMouthIndex(0); }
void UCustomizingWidget::OnClick_Mouth_1() { SetMouthIndex(1); }
void UCustomizingWidget::OnClick_Mouth_2() { SetMouthIndex(2); }
void UCustomizingWidget::OnClick_Mouth_3() { SetMouthIndex(3); }
void UCustomizingWidget::OnClick_Mouth_4() { SetMouthIndex(4); }

void UCustomizingWidget::OnClick_Save()
{
    UE_LOG(LogTemp, Warning, TEXT("💾 [Save] 저장 버튼 클릭됨!"));
    if (Ref_GameInstance)
    {
        Ref_GameInstance->SaveAllData();
        UGameplayStatics::OpenLevel(this, FName("MainMenuLevel"));
    }
}