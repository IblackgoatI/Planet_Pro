// PlanetPlayerController.cpp

#include "PlanetPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "EngineUtils.h" // 액터 찾기용 (TActorIterator)
#include "MyGameInstance.h" // 님의 게임 인스턴스 헤더 (이름 꼭 확인하세요!)
#include "Misc/OutputDeviceNull.h"

void APlanetPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// [1] 입장하자마자 눈 가리기 (가장 중요!)
	// Duration 0.0f = 즉시 암전. bHold = True (계속 유지)
	if (PlayerCameraManager)
	{
		PlayerCameraManager->StartCameraFade(1.0f, 1.0f, 0.0f, FLinearColor::Black, false, true);
	}

	// [2] 0.1초 뒤에 시간 설정 시작
	// 하늘 액터(BP)가 생성될 시간을 아주 잠깐 줍니다.
	FTimerHandle WaitHandle;
	GetWorldTimerManager().SetTimer(WaitHandle, this, &APlanetPlayerController::ForceSkyUpdate, 0.1f, false);
}

void APlanetPlayerController::ForceSkyUpdate()
{
	// 1. 저장된 시간 가져오기
	UMyGameInstance* GI = Cast<UMyGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (!GI) 
	{
		// 데이터 없으면 그냥 바로 화면 켬
		FadeInScreen();
		return;
	}

	float TargetTime = GI->SavedSkyTime;
	UE_LOG(LogTemp, Warning, TEXT("🚀 [C++] 저장된 시간 적용 시작: %f"), TargetTime);

	// 2. 월드에서 하늘 액터(BP_StylizedSky...) 찾기
	AActor* SkyActor = nullptr;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		// 이름에 "StylizedSky"가 들어가는 놈을 찾음
		if (It->GetName().Contains(TEXT("StylizedSky")))
		{
			SkyActor = *It;
			break;
		}
	}

	if (SkyActor)
	{
		// 3. [핵심] Reflection으로 변수값 강제 주입
		// 사진에 있던 "Current Time of Day" -> C++에서는 "CurrentTimeOfDay" (공백 제거)
		FProperty* TimeProp = SkyActor->GetClass()->FindPropertyByName(TEXT("CurrentTimeOfDay"));
		
		if (TimeProp)
		{
			// float형 변수 포인터 가져오기
			float* ValuePtr = TimeProp->ContainerPtrToValuePtr<float>(SkyActor);
			if (ValuePtr)
			{
				*ValuePtr = TargetTime; // 값 덮어쓰기 (순간이동)
				UE_LOG(LogTemp, Warning, TEXT("✅ [C++] 변수 강제 주입 성공!"));
			}
		}

		// 4. "UserConstructionScript" 강제 호출 (새로고침)
		// 이걸 불러야 변수 바뀐 게 화면에 반영됨
		FOutputDeviceNull Ar;
		SkyActor->CallFunctionByNameWithArguments(TEXT("UserConstructionScript"), Ar, nullptr, true);
	}

	// 3. 모든 작업 끝! 0.5초 뒤에 커튼 걷기
	FTimerHandle FadeHandle;
	GetWorldTimerManager().SetTimer(FadeHandle, this, &APlanetPlayerController::FadeInScreen, 0.5f, false);
}

void APlanetPlayerController::FadeInScreen()
{
	if (PlayerCameraManager)
	{
		// 검은색(1.0) -> 투명(0.0)으로 2초 동안 서서히 밝아짐
		PlayerCameraManager->StartCameraFade(1.0f, 0.0f, 2.0f, FLinearColor::Black, false, false);
		UE_LOG(LogTemp, Warning, TEXT("✨ [C++] 로딩 완료. 화면 개방."));
	}
}