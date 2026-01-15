#include "PlanetPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "EngineUtils.h" 
#include "MyGameInstance.h" 
#include "Misc/OutputDeviceNull.h" // CallFunctionByNameWithArguments용
#include "Blueprint/UserWidget.h"

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
	// 1. 방장(Authority) 체크
	if (!HasAuthority())
	{
		FadeInScreen();
		return;
	}

	// 2. GameInstance 체크
	UMyGameInstance* GI = Cast<UMyGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (!GI)
	{
		FadeInScreen();
		return;
	}

	// ==========================================================
	// [수정] if (데이터 없음) ~ else (데이터 있음) 명확하게 분리
	// ==========================================================
	if (GI->SavedSkyTime < 0.0f)
	{
		// [상황 A] 아직 데이터가 안 왔을 때 -> 재시도 예약
		UE_LOG(LogTemp, Warning, TEXT("⏳ [C++] 아직 PlayFab 데이터 없음 (또는 로그인 중). 1초 뒤 재시도..."));

		// 로드 요청 다시 보내기
		GI->LoadSkyTime();

		// 1초 뒤에 다시 이 함수 실행 (재귀 호출)
		FTimerHandle RetryHandle;
		GetWorldTimerManager().SetTimer(RetryHandle, this, &APlanetPlayerController::ForceSkyUpdate, 1.0f, false);
	}
	else
	{
		// [상황 B] 데이터가 도착했을 때 -> 시간 적용!
		float TargetTime = GI->SavedSkyTime;
		UE_LOG(LogTemp, Warning, TEXT("🚀 [C++] 드디어 데이터 도착! 시간 적용 시작: %f"), TargetTime);

		// 하늘 액터 찾기
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
			// 1. 변수 값 직접 주입
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

			// 2. SetNewTimeSmooth 강제 호출 (순간이동)
			FOutputDeviceNull Ar;
			FString Cmd = FString::Printf(TEXT("SetNewTimeSmooth %f 100000.0"), TargetTime);
			bool bResult = SkyActor->CallFunctionByNameWithArguments(*Cmd, Ar, nullptr, true);

			if (bResult)
			{
				UE_LOG(LogTemp, Warning, TEXT("⚡ [C++] SetNewTimeSmooth 강제 호출 성공!"));
			}
			else
			{
				SkyActor->CallFunctionByNameWithArguments(TEXT("UpdateSun"), Ar, nullptr, true);
				SkyActor->CallFunctionByNameWithArguments(TEXT("UserConstructionScript"), Ar, nullptr, true);
			}
		}

		// 화면 개방 (로딩 끝!)
		FTimerHandle FadeHandle;
		GetWorldTimerManager().SetTimer(FadeHandle, this, &APlanetPlayerController::FadeInScreen, 0.5f, false);
	}
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

void APlanetPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// "Pause"라는 이름의 키 입력(Action)이 들어오면 TogglePauseMenu 함수 실행
	// (주의: 에디터 프로젝트 세팅에서 'Pause' 키를 등록해야 함. 아래 설명 참고)
	InputComponent->BindAction("Pause", IE_Pressed, this, &APlanetPlayerController::TogglePauseMenu);
}

void APlanetPlayerController::TogglePauseMenu()
{
	// 1. 설정된 위젯 클래스가 없으면 중단 (에디터에서 설정 안 했을 때 방지)
	if (!PauseMenuClass) 
	{
		UE_LOG(LogTemp, Error, TEXT("❌ PauseMenuClass가 비어있습니다! PC 블루프린트에서 위젯을 넣어주세요."));
		return;
	}

	// 2. 이미 메뉴가 켜져 있다면? -> 끄기 (Resume)
	if (PauseMenuInstance && PauseMenuInstance->IsInViewport())
	{
		PauseMenuInstance->RemoveFromParent();
		PauseMenuInstance = nullptr;

		// 마우스 숨기고 게임 모드로 복귀
		bShowMouseCursor = false;
		SetInputMode(FInputModeGameOnly());
	}
	// 3. 메뉴가 꺼져 있다면? -> 켜기 (Pause)
	else
	{
		// 위젯 생성
		PauseMenuInstance = CreateWidget<UUserWidget>(this, PauseMenuClass);
		if (PauseMenuInstance)
		{
			PauseMenuInstance->AddToViewport();

			// 마우스 보이게 하고 UI 조작 모드로 변경
			bShowMouseCursor = true;
            
			// UI랑 게임 둘 다 입력받게 하거나, UI만 받게 설정
			FInputModeGameAndUI InputMode;
			InputMode.SetWidgetToFocus(PauseMenuInstance->TakeWidget());
			SetInputMode(InputMode);
		}
	}
}