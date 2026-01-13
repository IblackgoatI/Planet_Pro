#include "MainHUDWidget.h"
#include "ItemSlotWidget.h"
#include "MyGameInstance.h"
#include "Components/HorizontalBox.h" 
#include "Components/UniformGridSlot.h" // [필수] 행/열 계산을 위해 필요
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UnrealType.h"
#include "Misc/OutputDeviceNull.h"

void UMainHUDWidget::ToggleInventory()
{
    if (!InventoryWindow) return;

    bIsInventoryOpen = !bIsInventoryOpen;
    
    APlayerController* PC = GetOwningPlayer();

    if (bIsInventoryOpen)
    {
        InventoryWindow->SetVisibility(ESlateVisibility::Visible);
        if (PC)
        {
            PC->bShowMouseCursor = true;
            PC->SetInputMode(FInputModeGameAndUI());
        }
    }
    else
    {
        InventoryWindow->SetVisibility(ESlateVisibility::Hidden);
        if (PC)
        {
            PC->bShowMouseCursor = false;
            PC->SetInputMode(FInputModeGameOnly());
        }
    }
}

void UMainHUDWidget::RefreshInventory(const TArray<FPlanetItemInfo>& InventoryItems)
{
    // =========================================================
    // 1. 퀵슬롯 업데이트 (가로 1줄, 0 ~ 9번)
    // =========================================================
    if (QuickSlotBar)
    {
        TArray<UWidget*> QSlots = QuickSlotBar->GetAllChildren();
        
        for (int32 i = 0; i < QSlots.Num(); i++)
        {
            UItemSlotWidget* SlotWidget = Cast<UItemSlotWidget>(QSlots[i]);
            if (SlotWidget)
            {
                SlotWidget->MyIndex = i; // 퀵슬롯 번호 (0~9)

                if (InventoryItems.IsValidIndex(i))
                {
                    SlotWidget->UpdateSlot(InventoryItems[i].ItemID, InventoryItems[i].Amount);
                }
                else
                {
                    SlotWidget->UpdateSlot(FName("None"), 0);
                }
            }
        }
    }

    // =========================================================
    // 2. 메인 인벤토리 업데이트 (행/열 계산 방식)
    // =========================================================
    if (InventoryGrid)
    {
        TArray<UWidget*> GSlots = InventoryGrid->GetAllChildren();
        
        // 퀵슬롯 개수 (0~9번까지 10개)
        const int32 QuickSlotCount = 10; 
        
        // [중요 체크] 본인 UI의 가로 칸 수에 맞춰주세요! (사진상으로는 6칸, 코드엔 8칸으로 적으셨음)
        // 만약 가로가 6칸이면 6으로, 8칸이면 8로 고치세요.
        const int32 ColumnsPerRow = 8; 

        for (UWidget* Widget : GSlots)
        {
            UItemSlotWidget* SlotWidget = Cast<UItemSlotWidget>(Widget);
            if (SlotWidget)
            {
                // 1. 이 슬롯이 그리드 몇 번째 줄, 몇 번째 칸에 있는지 알아냄
                UUniformGridSlot* GridSlot = Cast<UUniformGridSlot>(SlotWidget->Slot);
                
                int32 FinalIndex = -1;

                if (GridSlot)
                {
                    int32 Row = GridSlot->GetRow();
                    int32 Col = GridSlot->GetColumn();

                    // 2. 수학 공식으로 번호 계산: (줄 번호 * 칸수) + 칸 번호 + 퀵슬롯개수
                    FinalIndex = (Row * ColumnsPerRow) + Col + QuickSlotCount;
                }
                else
                {
                    continue; 
                }

                // 3. 번호표 부여 (드래그 앤 드롭의 핵심!)
                SlotWidget->MyIndex = FinalIndex;

                // 4. 아이템 표시
                if (InventoryItems.IsValidIndex(FinalIndex))
                {
                    SlotWidget->UpdateSlot(InventoryItems[FinalIndex].ItemID, InventoryItems[FinalIndex].Amount);
                }
                else
                {
                    SlotWidget->UpdateSlot(FName("None"), 0);
                }
            }
        }
    }
}

void UMainHUDWidget::UpdateQuickSlotHighlight(int32 SelectedIndex)
{
    if (!QuickSlotBar) return;

    TArray<UWidget*> QSlots = QuickSlotBar->GetAllChildren();

    // 모든 퀵슬롯을 하나씩 검사
    for (int32 i = 0; i < QSlots.Num(); i++)
    {
        UItemSlotWidget* SlotWidget = Cast<UItemSlotWidget>(QSlots[i]);
        if (SlotWidget)
        {
            // "지금 검사하는 번호(i)"가 "선택된 번호(SelectedIndex)"랑 같니?
            if (i == SelectedIndex)
            {
                SlotWidget->SetIsSelected(true);  // 너는 커져라!
            }
            else
            {
                SlotWidget->SetIsSelected(false); // 너는 작아져라!
            }
        }
    }
}

void UMainHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 1. 하늘 액터 찾기
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), FoundActors);

    for (AActor* Actor : FoundActors)
    {
        if (Actor->GetName().Contains(TEXT("StylizedSky")))
        {
               SkyActor = Actor;
            break;
        }
    }

    // 2. [중요] 게임 켜질 때 시간 강제 설정 (아침 8시)
    // 저장된 데이터가 0으로 만들든 말든, 여기서 800으로 덮어씁니다.
    InternalGameTime = 800.0f; 
    
    GetWorld()->GetTimerManager().SetTimer(TimerHandle_AutoSave, this, &UMainHUDWidget::AutoSaveTime, 60.0f, true);
    
}

void UMainHUDWidget::UpdateGameTime()
{
    if (!SkyActor || !Txt_GameTime || !Img_SunMoon) return;

    // -------------------------------------------------------
    // [1] 하늘 에셋의 진짜 시간(Current Time of Day) 훔쳐오기
    // -------------------------------------------------------
    float CurrentSkyTime = 0.0f;

    // ★수정됨★: 스크린샷에서 확인한 정확한 변수 이름 "Current Time of Day"를 넣었습니다.
    FProperty* Prop = SkyActor->GetClass()->FindPropertyByName(TEXT("Current Time of Day"));
    
    if (Prop)
    {
        // 변수 값을 읽어옵니다 (리포터 모드!)
        if (FNumericProperty* NumericProp = CastField<FNumericProperty>(Prop))
        {
            void* ValuePtr = NumericProp->ContainerPtrToValuePtr<void>(SkyActor);
            if (NumericProp->IsFloatingPoint())
            {
                CurrentSkyTime = NumericProp->GetFloatingPointPropertyValue(ValuePtr);
            }
        }
    }
    else
    {
        // 혹시 못 찾으면 로그 띄우기 (디버깅용)
        GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, TEXT("변수 못 찾음! 'Current Time of Day' 확인 필요"));
    }

    // -------------------------------------------------------
    // [2] UI 표시 (비율 계산 및 텍스트 변환)
    // -------------------------------------------------------
    // 테스트 맵 설정(Day: 50 + Night: 40 = 총 90)에 맞춰서 24시간제로 변환합니다.
    
    // [비율 계산]
    float TotalCycleDuration = 90.0f; // Day(50) + Night(40)
    float NormalizedTime24 = (CurrentSkyTime / TotalCycleDuration) * 24.0f;

    // ★수정 포인트★: 하늘 에셋의 0.0은 0시가 아니라 "아침 6시"입니다.
    // 이제 에디터에서 이 값을 0으로 하면 "0.0 = 자정", 6으로 하면 "0.0 = 6시"가 됩니다.
    NormalizedTime24 += TimeOffsetHours;

    // 24시 넘으면 0시부터 다시 시작 (예: 25시 -> 1시)
    if (NormalizedTime24 >= 24.0f)
    {
        NormalizedTime24 -= 24.0f;
    }

    // 1. 시(Hour)
    int32 Hour24 = (int32)NormalizedTime24;
    
    // 2. 분(Minute)
    float MinuteFloat = (NormalizedTime24 - Hour24) * 60.0f;
    int32 Minute = (int32)MinuteFloat;

    // 3. 초(Second)
    int32 Second = (int32)((MinuteFloat - Minute) * 60.0f);

    // 24시 넘는 경우 방어
    Hour24 = Hour24 % 24;

    // 4. AM/PM 변환
    int32 Hour12 = Hour24;
    FString Period = TEXT("am");
    
    if (Hour24 >= 12)
    {
        Period = TEXT("pm");
        if (Hour24 > 12) Hour12 -= 12; // 13시 -> 1시
    }
    if (Hour12 == 0) Hour12 = 12;

    // 텍스트 출력
    FString TimeString = FString::Printf(TEXT("%02d:%02d"), Hour12, Minute);
    if (Txt_GameTime)
    {
        Txt_GameTime->SetText(FText::FromString(TimeString));
    }

    // 2. AM/PM 상자에는 "pm" 만 넣음 (얘는 이제 안 움직임!)
    if (Txt_AmPm)
    {
        Txt_AmPm->SetText(FText::FromString(Period));
    }

    // 아이콘 변경
    if (Hour24 >= 6 && Hour24 < 20)
    {
        if (Icon_Sun && Img_SunMoon->GetBrush().GetResourceObject() != Icon_Sun)
            Img_SunMoon->SetBrushFromTexture(Icon_Sun);
    }
    else
    {
        if (Icon_Moon && Img_SunMoon->GetBrush().GetResourceObject() != Icon_Moon)
            Img_SunMoon->SetBrushFromTexture(Icon_Moon);
    }
}
// [빠진 함수 추가] 틱(매 프레임) 함수
void UMainHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // 필요한 경우 여기에 매 프레임 실행할 로직 추가 (없어도 이 함수는 있어야 함)
    UpdateGameTime();
}

void UMainHUDWidget::AutoSaveTime()
{
    // 하늘 액터가 없으면 저장 불가
    if (!SkyActor) return;

    // 1. 하늘 액터에서 현재 시간 값(float) 읽어오기
    float CurrentSkyTime = -1.0f;
    FProperty* Prop = SkyActor->GetClass()->FindPropertyByName(TEXT("Current Time of Day"));
    
    if (Prop)
    {
        if (FNumericProperty* NumericProp = CastField<FNumericProperty>(Prop))
        {
            void* ValuePtr = NumericProp->ContainerPtrToValuePtr<void>(SkyActor);
            if (NumericProp->IsFloatingPoint())
            {
                CurrentSkyTime = NumericProp->GetFloatingPointPropertyValue(ValuePtr);
            }
        }
    }

    // 값을 못 읽었으면 중단
    if (CurrentSkyTime < 0.0f) return;

    // 2. GameInstance에 저장 요청 보내기
    UMyGameInstance* GI = Cast<UMyGameInstance>(GetGameInstance());
    if (GI)
    {
        GI->SaveSkyTime(CurrentSkyTime);
        // (선택사항) 화면에 로그 띄우기 - 너무 자주 뜨면 주석 처리하세요
        // if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("Auto Saving Time..."));
    }
}