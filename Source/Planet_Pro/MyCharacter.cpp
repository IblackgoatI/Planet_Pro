#include "MyCharacter.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Json.h"
#include "JsonUtilities.h"

// [핵심] 이 3개의 헤더가 꼭 있어야 합니다!
#include "PlayFab.h"                       // 모듈 인터페이스 (GetClientAPI 용)
#include "Core/PlayFabClientDataModels.h"  // 데이터 모델 (Request 용)
#include "Core/PlayFabClientAPI.h"         // API 클래스 (Delegate 용)

AMyCharacter::AMyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
}

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
	/*// [긴급 복구용] 인벤토리 초기화 코드
	// ※ 주의: 게임 실행 후 인벤토리가 0이 된 걸 확인하면, 이 코드는 다시 지우거나 주석 처리하세요!

	// 1. 배열 싹 비우기
	Inventory.Empty();

	// 2. 서버에도 빈 배열로 덮어쓰기 (저장)
	SaveInventoryToPlayFab();

	UE_LOG(LogTemp, Warning, TEXT("🧹 [긴급] 인벤토리를 강제로 초기화했습니다! (쓰레기 값 제거 완료)"));

	// 3. (선택) UI도 갱신해서 눈으로 확인
	if (MainHUDInstance)
	{
		MainHUDInstance->RefreshInventory(Inventory);
	}*/
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

void AMyCharacter::AddInventoryItem(FName NewItemID, int32 NewAmount)
{
	if (NewAmount <= 0) return;

	// 1. [기존 로직] 이미 가지고 있는 아이템인가? (스택 합치기)
	for (int32 i = 0; i < Inventory.Num(); i++)
	{
		if (Inventory[i].ItemID == NewItemID)
		{
			Inventory[i].Amount += NewAmount;
            
			// UI 갱신 & 저장
			if (MainHUDInstance) MainHUDInstance->RefreshInventory(Inventory);
			SaveInventoryToPlayFab();
			return; // 함수 종료
		}
	}

	// =============================================================
	// 2. [신규 로직] 빈 자리(None)가 있는가? (구멍 메우기)
	// =============================================================
	for (int32 i = 0; i < Inventory.Num(); i++)
	{
		// 이름이 "None"이거나 수량이 0인 곳을 찾음
		if (Inventory[i].ItemID == FName("None") || Inventory[i].Amount <= 0)
		{
			Inventory[i].ItemID = NewItemID;
			Inventory[i].Amount = NewAmount;

			UE_LOG(LogTemp, Warning, TEXT("✨ 빈 슬롯(%d번)에 아이템을 채워 넣었습니다!"), i);

			// UI 갱신 & 저장
			if (MainHUDInstance) MainHUDInstance->RefreshInventory(Inventory);
			SaveInventoryToPlayFab();
			return; // 함수 종료
		}
	}

	// =============================================================
	// 3. [기존 로직] 빈자리도 없다면? (새 칸 늘리기)
	// =============================================================
	FPlanetItemInfo NewItem;
	NewItem.ItemID = NewItemID;
	NewItem.Amount = NewAmount;
	Inventory.Add(NewItem);

	// UI 갱신 & 저장
	if (MainHUDInstance) MainHUDInstance->RefreshInventory(Inventory);
	SaveInventoryToPlayFab();
}

/*void AMyCharacter::AddTestItem()
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
}*/

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
    // 1. PlayFab API 담당자 가져오기
    auto ClientAPI = IPlayFabModuleInterface::Get().GetClientAPI();

    // [방어 코드] API가 없거나 로그인이 안 된 경우 -> 즉시 로그인 시도!
    if (!ClientAPI.IsValid() || !ClientAPI->IsClientLoggedIn())
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ [PlayFab] 로그인 없이 저장을 시도했습니다. 긴급 로그인을 진행합니다..."));

        PlayFab::ClientModels::FLoginWithCustomIDRequest Request;
        Request.CustomId = TEXT("TestUser_01"); // 테스트용 ID
        Request.CreateAccount = true;

        if (ClientAPI.IsValid())
        {
            ClientAPI->LoginWithCustomID(Request,
                PlayFab::UPlayFabClientAPI::FLoginWithCustomIDDelegate::CreateLambda(
                    [this](const PlayFab::ClientModels::FLoginResult& Result)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("✅ [PlayFab] 긴급 로그인 성공! 다시 저장을 시도합니다."));
                        // 로그인 성공했으니, 재귀적으로 다시 저장 함수 호출
                        this->SaveInventoryToPlayFab(); 
                    }
                ),
                PlayFab::FPlayFabErrorDelegate::CreateLambda(
                    [](const PlayFab::FPlayFabCppError& ErrorResult)
                    {
                        UE_LOG(LogTemp, Error, TEXT("❌ [PlayFab] 긴급 로그인 실패: %s"), *ErrorResult.ErrorMessage);
                    }
                )
            );
        }
        return; // 로그인이 끝나면 다시 들어올 테니 여기서 함수 종료
    }

    // =======================================================
    // 2. 여기서부터는 기존 저장 로직 (로그인 된 상태)
    // =======================================================

    // JSON 데이터 만들기
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

    UE_LOG(LogTemp, Warning, TEXT("🚩 [PlayFab] 인벤토리 데이터 생성 완료: %s"), *OutputString);

    // PlayFab 요청
    PlayFab::ClientModels::FUpdateUserDataRequest Request;
    Request.Data.Add(TEXT("Inventory"), OutputString);

    ClientAPI->UpdateUserData(
       Request,
       PlayFab::UPlayFabClientAPI::FUpdateUserDataDelegate::CreateLambda(
          [this](const PlayFab::ClientModels::FUpdateUserDataResult& Result)
          {
             UE_LOG(LogTemp, Warning, TEXT("✅ [PlayFab] 서버 저장 성공!!!"));
             this->OnSaveSuccess(Result);
          }
       ),
       PlayFab::FPlayFabErrorDelegate::CreateLambda(
          [this](const PlayFab::FPlayFabCppError& ErrorResult)
          {
             UE_LOG(LogTemp, Error, TEXT("❌ [PlayFab] 서버 저장 실패: %s"), *ErrorResult.ErrorMessage);
             this->OnSaveError(ErrorResult);
          }
       )
    );
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

void AMyCharacter::SwapInventoryItems(int32 SourceIndex, int32 DestinationIndex)
{
	// 1. 유효성 체크
	if (SourceIndex == DestinationIndex) return;
	if (SourceIndex < 0 || DestinationIndex < 0) return;

	// 2. 배열 확장 및 "청소" (가장 중요한 수정 부분!)
	int32 MaxIndex = FMath::Max(SourceIndex, DestinationIndex);
    
	if (Inventory.Num() <= MaxIndex)
	{
		int32 OldSize = Inventory.Num();
		int32 NewSize = MaxIndex + 1;

		// 방 늘리기
		Inventory.SetNum(NewSize);

		// [중요] 새로 생긴 방들은 "쓰레기 값"이 들어있으므로, 싹 다 0으로 초기화!
		for (int32 i = OldSize; i < NewSize; i++)
		{
			Inventory[i].ItemID = FName("None"); // 이름 없음
			Inventory[i].Amount = 0;             // 수량 0 (이게 없어서 10억이 뜬 것!)
		}
	}

	// 3. 이제 깨끗한 방에서 안전하게 교환
	Inventory.Swap(SourceIndex, DestinationIndex);

	// 4. UI 갱신
	if (MainHUDInstance)
	{
		MainHUDInstance->RefreshInventory(Inventory);
	}

	// 5. 저장
	SaveInventoryToPlayFab();
    
	UE_LOG(LogTemp, Warning, TEXT("✨ [성공] %d번 <-> %d번 교체 완료. (배열 크기: %d)"), SourceIndex, DestinationIndex, Inventory.Num());
}