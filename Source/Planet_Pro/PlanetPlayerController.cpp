// PlanetPlayerController.cpp

#include "PlanetPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "EngineUtils.h" // TActorIterator
#include "MyGameInstance.h" // 헤더 이름 꼭 확인!
#include "Misc/OutputDeviceNull.h"

void APlanetPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// [1] 입장하자마자 눈 가리기 (암전)
	if (PlayerCameraManager)
	{
		PlayerCameraManager->StartCameraFade(1.0f, 1.0f, 0.0f, FLinearColor::Black, false, true);
	}

	// [2] 0.1초 뒤에 시간 설정 시도
	FTimerHandle WaitHandle;
	GetWorldTimerManager().SetTimer(WaitHandle, this, &APlanetPlayerController::ForceSkyUpdate, 0.1f, false);
}

void APlanetPlayerController::ForceSkyUpdate()
{
    // 1. GameInstance 확인 및 데이터 로드 요청 (기존 로직 유지)
    UMyGameInstance* GI = Cast<UMyGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
    if (!GI) 
    {
        FadeInScreen();
        return;
    }

    if (GI->SavedSkyTime < 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("⏳ [C++] 아직 시간이 로드되지 않음. 대기합니다..."));
        GI->OnSkyTimeLoaded.RemoveDynamic(this, &APlanetPlayerController::OnSkyTimeLoadedReceived);
        GI->OnSkyTimeLoaded.AddDynamic(this, &APlanetPlayerController::OnSkyTimeLoadedReceived);
        GI->LoadSkyTime();
        return; 
    }

    float TargetTime = GI->SavedSkyTime;
    UE_LOG(LogTemp, Warning, TEXT("🚀 [C++] 저장된 시간 적용 시작: %f"), TargetTime);

    // 2. 하늘 액터 찾기
    AActor* SkyActor = nullptr;
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        if (It->GetName().Contains(TEXT("StylizedSky")))
        {
            SkyActor = *It;
            break;
        }
    }

    if (SkyActor)
    {
        // =========================================================
        // [1] 안전장치: 변수 값 직접 주입 (혹시 모르니 해둠)
        // =========================================================
        FProperty* FoundProp = SkyActor->GetClass()->FindPropertyByName(TEXT("CurrentTimeOfDay"));
        if (!FoundProp) FoundProp = SkyActor->GetClass()->FindPropertyByName(TEXT("Current Time of Day"));
        
        if (FoundProp)
        {
            if (FNumericProperty* NumProp = CastField<FNumericProperty>(FoundProp))
            {
                if (NumProp->IsFloatingPoint())
                {
                    void* ValuePtr = NumProp->ContainerPtrToValuePtr<void>(SkyActor);
                    NumProp->SetFloatingPointPropertyValue(ValuePtr, TargetTime);
                    UE_LOG(LogTemp, Warning, TEXT("✅ [C++] 변수 값 직접 주입 완료."));
                }
            }
        }

        // =========================================================
        // [2] ★핵심 해결책★: 사진에 있던 함수 강제 실행!
        // 함수명: SetNewTimeSmooth
        // 인자 1: New Time (TargetTime)
        // 인자 2: Smooth Speed (100000.0 -> 엄청 빠르게 줘서 즉시이동 효과)
        // =========================================================
        
        FOutputDeviceNull Ar;
        
        // 명령문 만들기: "함수이름 값1 값2"
        FString Cmd = FString::Printf(TEXT("SetNewTimeSmooth %f 100000.0"), TargetTime);
        
        // 실행!
        bool bResult = SkyActor->CallFunctionByNameWithArguments(*Cmd, Ar, nullptr, true);

        if (bResult)
        {
            UE_LOG(LogTemp, Warning, TEXT("⚡ [C++] SetNewTimeSmooth 강제 호출 성공! (순간이동)"));
        }
        else
        {
            // 혹시 함수 이름이 다를까봐 다른 후보들도 찔러봅니다.
            SkyActor->CallFunctionByNameWithArguments(TEXT("UserConstructionScript"), Ar, nullptr, true);
            SkyActor->CallFunctionByNameWithArguments(TEXT("UpdateSun"), Ar, nullptr, true);
            UE_LOG(LogTemp, Warning, TEXT("⚠️ [C++] SetNewTimeSmooth 호출 실패. 대신 ConstructionScript 실행함."));
        }
    }

    // 3. 화면 개방 (페이드 인)
    FTimerHandle FadeHandle;
    GetWorldTimerManager().SetTimer(FadeHandle, this, &APlanetPlayerController::FadeInScreen, 0.5f, false);
}