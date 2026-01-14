#include "PlanetPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "EngineUtils.h" 
#include "MyGameInstance.h" 
#include "Misc/OutputDeviceNull.h" // CallFunctionByNameWithArguments용

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
	if (!HasAuthority()) 
	{
		// 단, 클라이언트도 화면은 밝혀줘야 게임을 하겠죠?
		FadeInScreen(); 
		return; 
	}
	
	// 1. GameInstance 확인
	UMyGameInstance* GI = Cast<UMyGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (!GI) 
	{
		FadeInScreen();
		return;
	}

	// 2. 데이터가 아직 없다면? (-1.0f) -> 요청 보내기
	if (GI->SavedSkyTime < 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("⏳ [C++] 아직 시간이 로드되지 않음. 대기합니다..."));
		
		// 방송 수신 대기
		GI->OnSkyTimeLoaded.RemoveDynamic(this, &APlanetPlayerController::OnSkyTimeLoadedReceived);
		GI->OnSkyTimeLoaded.AddDynamic(this, &APlanetPlayerController::OnSkyTimeLoadedReceived);
		
		// ★ 데이터 요청!
		GI->LoadSkyTime(); 
		return; 
	}

	float TargetTime = GI->SavedSkyTime;
	UE_LOG(LogTemp, Warning, TEXT("🚀 [C++] 저장된 시간 적용 시작: %f"), TargetTime);

	// 3. 하늘 액터 찾기
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
		// [1] 안전장치: 변수 값 직접 주입
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
				}
			}
		}

		// [2] ★핵심★: SetNewTimeSmooth 강제 실행 (순간이동)
		FOutputDeviceNull Ar;
		FString Cmd = FString::Printf(TEXT("SetNewTimeSmooth %f 100000.0"), TargetTime);
		bool bResult = SkyActor->CallFunctionByNameWithArguments(*Cmd, Ar, nullptr, true);

		if (bResult)
		{
			UE_LOG(LogTemp, Warning, TEXT("⚡ [C++] SetNewTimeSmooth 강제 호출 성공!"));
		}
		else
		{
			// 실패 시 대체 수단들
			SkyActor->CallFunctionByNameWithArguments(TEXT("UserConstructionScript"), Ar, nullptr, true);
			SkyActor->CallFunctionByNameWithArguments(TEXT("UpdateSun"), Ar, nullptr, true);
		}
	}

	// 4. 화면 개방
	FTimerHandle FadeHandle;
	GetWorldTimerManager().SetTimer(FadeHandle, this, &APlanetPlayerController::FadeInScreen, 0.5f, false);
}

// ★ 누락되었던 함수 1: 데이터 도착하면 다시 실행
void APlanetPlayerController::OnSkyTimeLoadedReceived(float LoadedTime)
{
	UE_LOG(LogTemp, Warning, TEXT("📨 [C++] 데이터 도착 알림 받음! 다시 시간 설정 시도."));
	ForceSkyUpdate();
}

// ★ 누락되었던 함수 2: 화면 밝히기
void APlanetPlayerController::FadeInScreen()
{
	if (PlayerCameraManager)
	{
		PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 2.0f, FLinearColor::Black, false, false);
		UE_LOG(LogTemp, Warning, TEXT("✨ [C++] 로딩 완료. 화면 개방."));
	}
}