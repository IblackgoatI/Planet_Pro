#include "MyCharacter.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Json.h"
#include "JsonUtilities.h"

// [핵심] 이 3개의 헤더가 꼭 있어야 합니다!
#include "PlayFab.h"                       // 모듈 인터페이스 (GetClientAPI 용)
#include "Core/PlayFabClientDataModels.h"  // 데이터 모델 (Request 용)
#include "Core/PlayFabClientAPI.h"         // API 클래스 (Delegate 용)


void AMyCharacter::BeginPlay()
{
    Super::BeginPlay();

    // 1. UI 생성 로직 (기존 코드 유지)
    FString CurrentMapName = GetWorld()->GetMapName();
    if (!CurrentMapName.Contains("Lobby") && MainHUDClass)
    {
       APlayerController* PC = Cast<APlayerController>(GetController());
       if (PC)
       {
          MainHUDInstance = CreateWidget<UMainHUDWidget>(PC, MainHUDClass);
          if (MainHUDInstance)
          {
             MainHUDInstance->AddToViewport();
             if (MainHUDInstance->InventoryWindow)
             {
                MainHUDInstance->InventoryWindow->SetVisibility(ESlateVisibility::Hidden);
             }
          }
       }
    }

    // =================================================================
    // [추가된 부분] 테스트용 자동 로그인 코드
    // =================================================================
    
    // 1. PlayFab 모듈 가져오기
    auto ClientAPI = IPlayFabModuleInterface::Get().GetClientAPI();

    // 2. 이미 로그인 되어 있는지 확인 (로비에서 넘어왔으면 true일 것임)
    if (ClientAPI.IsValid() && ClientAPI->IsClientLoggedIn())
    {
        UE_LOG(LogTemp, Warning, TEXT("✅ [PlayFab] 이미 로그인 상태입니다. 인벤토리를 불러옵니다."));
        LoadInventoryFromPlayFab();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ [PlayFab] 로그인 안 됨! 테스트용 자동 로그인을 시도합니다..."));

        PlayFab::ClientModels::FLoginWithCustomIDRequest Request;
        Request.CustomId = TEXT("TestUser_01"); // 테스트용 아이디 (아무거나 적어도 됨)
        Request.CreateAccount = true;           // 계정 없으면 새로 만들기

        ClientAPI->LoginWithCustomID(Request,
            // 로그인 성공 시
            PlayFab::UPlayFabClientAPI::FLoginWithCustomIDDelegate::CreateLambda(
                [this](const PlayFab::ClientModels::FLoginResult& Result)
                {
                    UE_LOG(LogTemp, Warning, TEXT("✅ [PlayFab] 자동 로그인 성공! 이제 인벤토리를 불러옵니다."));
                    this->LoadInventoryFromPlayFab(); // 로그인 성공 후에 불러오기 실행!
                }
            ),
            // 로그인 실패 시
            PlayFab::FPlayFabErrorDelegate::CreateLambda(
                [](const PlayFab::FPlayFabCppError& ErrorResult)
                {
                    UE_LOG(LogTemp, Error, TEXT("❌ [PlayFab] 자동 로그인 실패: %s"), *ErrorResult.ErrorMessage);
                }
            )
        );
    }
}

void AMyCharacter::OnInventoryKeyPressed()
{
	if (MainHUDInstance)
	{
		MainHUDInstance->ToggleInventory();
		if (MainHUDInstance->bIsInventoryOpen)
		{
			MainHUDInstance->RefreshInventory(Inventory);
		}
	}
}

void AMyCharacter::AddTestItem()
{
	// [아이템 추가]
	bool bFound = false;
	for (FPlanetItemInfo& Item : Inventory)
	{
		if (Item.ItemID == "Wood")
		{
			Item.Amount++;
			bFound = true;
			break;
		}
	}

	if (!bFound)
	{
		FPlanetItemInfo NewItem;
		NewItem.ItemID = "Wood";
		NewItem.Amount = 1;
		Inventory.Add(NewItem);
	}

	// [UI 갱신]
	if (MainHUDInstance)
	{
		MainHUDInstance->RefreshInventory(Inventory);
	}

	// [자동 저장]
	SaveInventoryToPlayFab();
}

/*void AMyCharacter::SaveInventoryToPlayFab()
{
	// 1. JSON 데이터 만들기 (이 부분은 완벽합니다!)
	TArray<TSharedPtr<FJsonValue>> JsonItemsArray;
	for (const FPlanetItemInfo& Item : Inventory)
	{
		TSharedPtr<FJsonObject> ItemObject = MakeShareable(new FJsonObject);
		ItemObject->SetStringField("ItemID", Item.ItemID.ToString());
		ItemObject->SetNumberField("Amount", Item.Amount);

		TSharedPtr<FJsonValueObject> JsonValue = MakeShareable(new FJsonValueObject(ItemObject));
		JsonItemsArray.Add(JsonValue);
	}

	TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
	RootObject->SetArrayField("Items", JsonItemsArray);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

	// 2. PlayFab 요청 데이터 준비
	PlayFab::ClientModels::FUpdateUserDataRequest Request;
	Request.Data.Add("Inventory", OutputString);

	// ===========================================================================
	// [최종 해결] 델리게이트 이름 때문에 고생하지 말고 "람다"를 씁니다.
	// ===========================================================================

	// 1. API 담당자 가져오기
	auto ClientAPI = IPlayFabModuleInterface::Get().GetClientAPI();

	// 2. 실행
	if (ClientAPI.IsValid())
	{
		ClientAPI->UpdateUserData(
			Request,

			// [성공 시] 그 자리에서 바로 함수를 만들어서 실행 (람다)
			PlayFab::UPlayFabClientAPI::FUpdateUserDataDelegate::CreateLambda(
				[this](const PlayFab::ClientModels::FUpdateUserDataResult& Result)
				{
					this->OnSaveSuccess(Result);
				}
			),

			// [실패 시] 그 자리에서 바로 함수를 만들어서 실행 (람다)
			PlayFab::FPlayFabErrorDelegate::CreateLambda(
				[this](const PlayFab::FPlayFabCppError& ErrorResult)
				{
					this->OnSaveError(ErrorResult);
				}
			)
		);
	}
}*/

void AMyCharacter::SaveInventoryToPlayFab()
{
    // [추적 1] 함수 진입 확인
    UE_LOG(LogTemp, Warning, TEXT("🚩 [1] SaveInventoryToPlayFab 함수 시작됨!"));

    // 1. JSON 데이터 만들기
    TArray<TSharedPtr<FJsonValue>> JsonItemsArray;
    for (const FPlanetItemInfo& Item : Inventory)
    {
       TSharedPtr<FJsonObject> ItemObject = MakeShareable(new FJsonObject);
       ItemObject->SetStringField(TEXT("ItemID"), Item.ItemID.ToString());
       ItemObject->SetNumberField(TEXT("Amount"), Item.Amount);

       TSharedPtr<FJsonValueObject> JsonValue = MakeShareable(new FJsonValueObject(ItemObject));
       JsonItemsArray.Add(JsonValue);
    }

    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
    RootObject->SetArrayField(TEXT("Items"), JsonItemsArray);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

    // [추적 2] JSON 변환 확인
    UE_LOG(LogTemp, Warning, TEXT("🚩 [2] JSON 생성 완료: %s"), *OutputString);

    // 2. PlayFab 요청 데이터 준비
    PlayFab::ClientModels::FUpdateUserDataRequest Request;
    Request.Data.Add(TEXT("Inventory"), OutputString);

    // 3. API 담당자 가져오기
    auto ClientAPI = IPlayFabModuleInterface::Get().GetClientAPI();

    // 4. 실행
    if (ClientAPI.IsValid())
    {
       UE_LOG(LogTemp, Warning, TEXT("🚩 [3] ClientAPI 유효함. 서버로 전송 시도..."));

       ClientAPI->UpdateUserData(
          Request,
          PlayFab::UPlayFabClientAPI::FUpdateUserDataDelegate::CreateLambda(
             [this](const PlayFab::ClientModels::FUpdateUserDataResult& Result)
             {
                UE_LOG(LogTemp, Warning, TEXT("🚩 [4-성공] 서버 응답 옴: 저장 성공!"));
                this->OnSaveSuccess(Result);
             }
          ),
          PlayFab::FPlayFabErrorDelegate::CreateLambda(
             [this](const PlayFab::FPlayFabCppError& ErrorResult)
             {
                UE_LOG(LogTemp, Error, TEXT("🚩 [4-실패] 서버 응답 옴: 실패! 이유: %s"), *ErrorResult.ErrorMessage);
                this->OnSaveError(ErrorResult);
             }
          )
       );
    }
    else
    {
        // 🚨 여기가 범인일 확률 높음
        UE_LOG(LogTemp, Error, TEXT("🚨 [ERROR] ClientAPI가 유효하지 않습니다! (로그인이 안 됐거나, PlayFab 설정 누락)"));
    }
}

void AMyCharacter::OnSaveSuccess(const PlayFab::ClientModels::FUpdateUserDataResult& Result)
{
	UE_LOG(LogTemp, Log, TEXT("✅ [PlayFab] 저장 성공!"));
}

void AMyCharacter::OnSaveError(const PlayFab::FPlayFabCppError& ErrorResult)
{
	UE_LOG(LogTemp, Error, TEXT("❌ [PlayFab] 저장 실패: %s"), *ErrorResult.ErrorMessage);
}

void AMyCharacter::LoadInventoryFromPlayFab()
{
	// 1. 요청서 작성
	PlayFab::ClientModels::FGetUserDataRequest Request;
	Request.Keys.Add("Inventory");

	// 2. API 담당자 호출
	auto ClientAPI = IPlayFabModuleInterface::Get().GetClientAPI();

	if (ClientAPI.IsValid())
	{
		// 3. 데이터 주세요! (GetUserData)
		ClientAPI->GetUserData(
			Request,

			// [수정 1] ClientModels가 아니라 UPlayFabClientAPI 안에 있는 델리게이트를 써야 합니다.
			PlayFab::UPlayFabClientAPI::FGetUserDataDelegate::CreateLambda(
				[this](const PlayFab::ClientModels::FGetUserDataResult& Result)
				{
					this->OnLoadSuccess(Result);
				}
			),

			// [수정 2] 에러 델리게이트도 이름을 정확하게 (FPlayFabErrorDelegate)
			PlayFab::FPlayFabErrorDelegate::CreateLambda(
				[this](const PlayFab::FPlayFabCppError& ErrorResult)
				{
					this->OnLoadError(ErrorResult);
				}
			)
		);
	}
}

void AMyCharacter::OnLoadSuccess(const PlayFab::ClientModels::FGetUserDataResult& Result)
{
	// 1. "Inventory" 데이터가 있는지 확인
	if (Result.Data.Contains("Inventory"))
	{
		UE_LOG(LogTemp, Log, TEXT("✅ [PlayFab] 데이터 발견! 인벤토리 복구 중..."));

		// 2. JSON 문자열 꺼내기
		FString JsonString = Result.Data["Inventory"].Value;

		// 3. JSON -> 언리얼 데이터로 변환 (파싱)
		TSharedPtr<FJsonObject> RootObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

		if (FJsonSerializer::Deserialize(Reader, RootObject))
		{
			// 기존(빈) 인벤토리 비우기
			Inventory.Empty();

			// "Items" 배열 가져오기
			// [수정] TEXT() 매크로 추가
			TArray<TSharedPtr<FJsonValue>> JsonItems = RootObject->GetArrayField(TEXT("Items"));

			for (auto& Value : JsonItems)
			{
				TSharedPtr<FJsonObject> ItemObj = Value->AsObject();

				// 구조체 만들어서 채우기
				FPlanetItemInfo LoadedItem;
				// [수정] TEXT() 매크로 추가
				LoadedItem.ItemID = FName(*ItemObj->GetStringField(TEXT("ItemID")));
				LoadedItem.Amount = ItemObj->GetNumberField(TEXT("Amount"));

				// 내 인벤토리에 추가
				Inventory.Add(LoadedItem);
			}

			// 4. UI 갱신 (중요! 이게 없으면 데이터만 들어오고 화면은 그대로임)
			if (MainHUDInstance)
			{
				MainHUDInstance->RefreshInventory(Inventory);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("⚠️ [PlayFab] 저장된 인벤토리 데이터가 없습니다. (신규 유저?)"));
	}
}

void AMyCharacter::OnLoadError(const PlayFab::FPlayFabCppError& ErrorResult)
{
	UE_LOG(LogTemp, Error, TEXT("❌ [PlayFab] 불러오기 실패: %s"), *ErrorResult.ErrorMessage);
}