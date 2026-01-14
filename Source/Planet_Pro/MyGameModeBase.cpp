#include "MyGameModeBase.h"
#include "MyGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/OutputDeviceNull.h"

void AMyGameModeBase::StartPlay()
{
    Super::StartPlay();
    
    // 0.5초 딜레이 (안전하게)
    //FTimerHandle WaitHandle;
    //GetWorld()->GetTimerManager().SetTimer(WaitHandle, this, &AMyGameModeBase::ApplySavedTime, 0.5f, false);
}

void AMyGameModeBase::ApplySavedTime()
{

}