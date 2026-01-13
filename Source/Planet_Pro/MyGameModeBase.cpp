#include "MyGameModeBase.h"
#include "MyGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/OutputDeviceNull.h"

void AMyGameModeBase::StartPlay()
{
    Super::StartPlay();
    
    // 0.5초 딜레이 (안전하게)
    FTimerHandle WaitHandle;
    GetWorld()->GetTimerManager().SetTimer(WaitHandle, this, &AMyGameModeBase::ApplySavedTime, 0.5f, false);
}

void AMyGameModeBase::ApplySavedTime()
{
    UMyGameInstance* GI = Cast<UMyGameInstance>(GetGameInstance());
    if (!GI || GI->SavedSkyTime < 0.0f) return;

    // 1. 하늘 액터 찾기
    AActor* SkyActor = nullptr;
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

    if (!SkyActor) return;
    float LoadTime = GI->SavedSkyTime;

    // =========================================================
    // 2. [최종 해결책] 우리가 뚫어놓은 뒷문(Custom Event) 호출
    // =========================================================
    
    // 함수 이름: 아까 블루프린트에서 만든 "CPP_ForceUpdateTime"
    // 파라미터: LoadTime
    
    FOutputDeviceNull ar;
    FString Cmd = FString::Printf(TEXT("CPP_ForceUpdateTime %f"), LoadTime);
    
    bool bResult = SkyActor->CallFunctionByNameWithArguments(*Cmd, ar, nullptr, true);

    if (bResult)
    {
        UE_LOG(LogTemp, Warning, TEXT("🚀 [GameMode] 커스텀 이벤트(CPP_ForceUpdateTime) 호출 성공! 시간: %f"), LoadTime);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ [GameMode] 커스텀 이벤트 호출 실패! 블루프린트 함수 이름을 확인하세요."));
    }
}