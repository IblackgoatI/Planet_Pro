#include "MyCharacter.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Json.h"
#include "JsonUtilities.h"

// [핵심] 이 3개의 헤더가 꼭 있어야 합니다!
#include "PlayFab.h"                        // 모듈 인터페이스
#include "Core/PlayFabClientDataModels.h"   // 데이터 모델
#include "Core/PlayFabClientAPI.h"          // API 클래스

AMyCharacter::AMyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	/*Inventory.Empty();

	// 2. 서버에도 빈 배열로 덮어쓰기 (저장)
	SaveInventoryToPlayFab();

	UE_LOG(LogTemp, Warning, TEXT("🧹 [긴급] 인벤토리를 강제로 초기화했습니다!"));*/

	// [추가] 게임 시작 시 무기 숨기기 (기본 상태)
	if (WeaponMeshComp)
	{
		WeaponMeshComp->SetVisibility(false);
	}

	// 1. UI 생성 로직
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
	// 테스트용 자동 로그인 코드
	// =================================================================
	auto ClientAPI = IPlayFabModuleInterface::Get().GetClientAPI();

	if (ClientAPI.IsValid() && ClientAPI->IsClientLoggedIn())
	{
		UE_LOG(LogTemp, Warning, TEXT("✅ [PlayFab] 이미 로그인 상태입니다. 인벤토리를 불러옵니다."));
		LoadInventoryFromPlayFab();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("⚠️ [PlayFab] 로그인 안 됨! 테스트용 자동 로그인을 시도합니다..."));

		PlayFab::ClientModels::FLoginWithCustomIDRequest Request;
		FString PCName = FPlatformProcess::ComputerName();
		Request.CustomId = PCName; 

		//새로운 유저 추가
		Request.CreateAccount = true;
        
		UE_LOG(LogTemp, Warning, TEXT("💻 현재 컴퓨터 이름(%s)으로 로그인을 시도합니다."), *PCName);

		ClientAPI->LoginWithCustomID(Request,
			PlayFab::UPlayFabClientAPI::FLoginWithCustomIDDelegate::CreateLambda(
				[this](const PlayFab::ClientModels::FLoginResult& Result)
				{
					UE_LOG(LogTemp, Warning, TEXT("✅ [PlayFab] 자동 로그인 성공! 이제 인벤토리를 불러옵니다."));
					this->LoadInventoryFromPlayFab();
				}
			),
			PlayFab::FPlayFabErrorDelegate::CreateLambda(
				[](const PlayFab::FPlayFabCppError& ErrorResult)
				{
					UE_LOG(LogTemp, Error, TEXT("❌ [PlayFab] 자동 로그인 실패: %s"), *ErrorResult.ErrorMessage);
				}
			)
		);
	}
}

// ... (기존 OnInventoryKeyPressed, AddInventoryItem 등은 그대로 유지) ...

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

	// 1. 기존 아이템 합치기
	for (int32 i = 0; i < Inventory.Num(); i++)
	{
		if (Inventory[i].ItemID == NewItemID)
		{
			Inventory[i].Amount += NewAmount;
			if (MainHUDInstance) MainHUDInstance->RefreshInventory(Inventory);
			SaveInventoryToPlayFab();
			return;
		}
	}

	// 2. 빈 슬롯 채우기
	for (int32 i = 0; i < Inventory.Num(); i++)
	{
		if (Inventory[i].ItemID == FName("None") || Inventory[i].Amount <= 0)
		{
			Inventory[i].ItemID = NewItemID;
			Inventory[i].Amount = NewAmount;
			if (MainHUDInstance) MainHUDInstance->RefreshInventory(Inventory);
			SaveInventoryToPlayFab();
			return; 
		}
	}

	// 3. 새 칸 늘리기
	FPlanetItemInfo NewItem;
	NewItem.ItemID = NewItemID;
	NewItem.Amount = NewAmount;
	Inventory.Add(NewItem);

	if (MainHUDInstance) MainHUDInstance->RefreshInventory(Inventory);
	SaveInventoryToPlayFab();
}

// ... (SaveInventoryToPlayFab, LoadInventoryFromPlayFab 등 기존 함수 유지) ...

void AMyCharacter::SaveInventoryToPlayFab()
{
	auto ClientAPI = IPlayFabModuleInterface::Get().GetClientAPI();

	if (!ClientAPI.IsValid() || !ClientAPI->IsClientLoggedIn())
	{
		// ... (긴급 로그인 로직 생략, 기존 코드 유지) ...
        // (코드가 너무 길어지니 중략합니다. 기존에 작성하신 긴급 로그인 포함된 코드 그대로 두세요)
        return;
	}

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

	PlayFab::ClientModels::FUpdateUserDataRequest Request;
	Request.Data.Add(TEXT("Inventory"), OutputString);

	ClientAPI->UpdateUserData(
		Request,
		PlayFab::UPlayFabClientAPI::FUpdateUserDataDelegate::CreateLambda(
			[this](const PlayFab::ClientModels::FUpdateUserDataResult& Result)
			{
				this->OnSaveSuccess(Result);
			}
		),
		PlayFab::FPlayFabErrorDelegate::CreateLambda(
			[this](const PlayFab::FPlayFabCppError& ErrorResult)
			{
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
    // ... (기존 LoadInventoryFromPlayFab 코드 유지) ...
    // 내용이 길어서 생략하지만, 기존 코드를 그대로 쓰시면 됩니다.
    // 단, 불러오기 완료 시에도 무기 상태를 업데이트해주면 좋습니다.
    
    // (여기서는 편의상 함수 내용 생략, 기존 코드 그대로 사용하세요)
    PlayFab::ClientModels::FGetUserDataRequest Request;
	Request.Keys.Add("Inventory");
	auto ClientAPI = IPlayFabModuleInterface::Get().GetClientAPI();

	if (ClientAPI.IsValid())
	{
		ClientAPI->GetUserData(
			Request,
			PlayFab::UPlayFabClientAPI::FGetUserDataDelegate::CreateLambda(
				[this](const PlayFab::ClientModels::FGetUserDataResult& Result)
				{
					this->OnLoadSuccess(Result);
				}
			),
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
    // ... (기존 파싱 로직) ...
    if (Result.Data.Contains("Inventory"))
	{
		// ... JSON 파싱 로직 ...
        FString JsonString = Result.Data["Inventory"].Value;
		TSharedPtr<FJsonObject> RootObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

		if (FJsonSerializer::Deserialize(Reader, RootObject))
		{
			Inventory.Empty();
			TArray<TSharedPtr<FJsonValue>> JsonItems = RootObject->GetArrayField(TEXT("Items"));

			for (auto& Value : JsonItems)
			{
				TSharedPtr<FJsonObject> ItemObj = Value->AsObject();
				FPlanetItemInfo LoadedItem;
				LoadedItem.ItemID = FName(*ItemObj->GetStringField(TEXT("ItemID")));
				LoadedItem.Amount = ItemObj->GetNumberField(TEXT("Amount"));
				Inventory.Add(LoadedItem);
			}

			if (MainHUDInstance) MainHUDInstance->RefreshInventory(Inventory);
            
            // [추가] 로드된 인벤토리 상태에 맞춰 무기 비주얼 업데이트
            UpdateWeaponVisuals();
		}
	}
}

void AMyCharacter::OnLoadError(const PlayFab::FPlayFabCppError& ErrorResult)
{
	UE_LOG(LogTemp, Error, TEXT("❌ [PlayFab] 불러오기 실패: %s"), *ErrorResult.ErrorMessage);
}

void AMyCharacter::SwapInventoryItems(int32 SourceIndex, int32 DestinationIndex)
{
    // ... (기존 Swap 로직 유지) ...
	if (SourceIndex == DestinationIndex) return;
	if (SourceIndex < 0 || DestinationIndex < 0) return;

	int32 MaxIndex = FMath::Max(SourceIndex, DestinationIndex);
	if (Inventory.Num() <= MaxIndex)
	{
		int32 OldSize = Inventory.Num();
		int32 NewSize = MaxIndex + 1;
		Inventory.SetNum(NewSize);
		for (int32 i = OldSize; i < NewSize; i++)
		{
			Inventory[i].ItemID = FName("None"); 
			Inventory[i].Amount = 0;
		}
	}

	Inventory.Swap(SourceIndex, DestinationIndex);

	if (MainHUDInstance) MainHUDInstance->RefreshInventory(Inventory);
	SaveInventoryToPlayFab();

    // [추가] 스왑으로 인해 현재 든 무기가 바뀔 수 있으므로 비주얼 업데이트
    UpdateWeaponVisuals();
}

// =================================================================
// [새로 작성한 핵심 함수들]
// =================================================================

// 1. 퀵슬롯 선택 시 무기 비주얼 업데이트 추가
void AMyCharacter::SelectQuickSlot(int32 SlotIndex)
{
	CurrentSelectedSlotIndex = SlotIndex;
	
	if (MainHUDInstance)
	{
		MainHUDInstance->UpdateQuickSlotHighlight(CurrentSelectedSlotIndex);
	}

	UE_LOG(LogTemp, Log, TEXT("✅ 퀵슬롯 선택: %d번"), SlotIndex + 1);

    // [추가] 선택된 슬롯의 아이템에 따라 무기 표시/숨김
    UpdateWeaponVisuals();
}

// 2. 도끼 지급 치트 함수 (= 키)
void AMyCharacter::GetAxeCheat()
{
	FName AxeID = FName("Axe");

	// 1. 이미 도끼가 있는지 확인 (중복 지급 방지)
	for (const FPlanetItemInfo& Item : Inventory)
	{
		if (Item.ItemID == AxeID)
		{
			UE_LOG(LogTemp, Warning, TEXT("⚠️ 이미 도끼를 가지고 있습니다!"));
			return;
		}
	}

	// 2. 인벤토리 공간 확보
	if (Inventory.Num() <= 0)
	{
		Inventory.SetNum(1);
	}

	// =========================================================
	// [수정 핵심] 0번 자리에 있던 아이템 "이사" 보내기
	// =========================================================
	FPlanetItemInfo OldItem = Inventory[0]; // 0번 아이템을 잠깐 복사해둠
	bool bHasOldItem = (OldItem.ItemID != FName("None") && OldItem.Amount > 0);

	// 3. 0번 칸에 도끼 강제 입주
	Inventory[0].ItemID = AxeID;
	Inventory[0].Amount = 1;

	// 4. 아까 복사해둔 아이템이 있었다면, 다른 빈 칸을 찾아서 넣어줌
	if (bHasOldItem)
	{
		UE_LOG(LogTemp, Warning, TEXT("📦 원래 0번에 있던 %s 아이템을 다른 곳으로 옮깁니다."), *OldItem.ItemID.ToString());
        
		// 기존 AddInventoryItem 함수를 재활용하면 알아서 빈 곳에 넣어줍니다!
		AddInventoryItem(OldItem.ItemID, OldItem.Amount);
	}

	UE_LOG(LogTemp, Warning, TEXT("🪓 도끼가 0번 슬롯에 지급되었습니다!"));

	// 5. UI 갱신 & 저장 & 비주얼 업데이트
	if (MainHUDInstance) MainHUDInstance->RefreshInventory(Inventory);
	SaveInventoryToPlayFab();
    
	// 만약 1번 키(Index 0)를 선택 중이었다면 바로 도끼를 보여줌
	UpdateWeaponVisuals();
}

// 키 바인딩
void AMyCharacter::OnQuickSlot1() { SelectQuickSlot(0); }
void AMyCharacter::OnQuickSlot2() { SelectQuickSlot(1); }
void AMyCharacter::OnQuickSlot3() { SelectQuickSlot(2); }
void AMyCharacter::OnQuickSlot4() { SelectQuickSlot(3); }
void AMyCharacter::OnQuickSlot5() { SelectQuickSlot(4); }
void AMyCharacter::OnQuickSlot6() { SelectQuickSlot(5); }
void AMyCharacter::OnQuickSlot7() { SelectQuickSlot(6); }
void AMyCharacter::OnQuickSlot8() { SelectQuickSlot(7); }

void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

    // [기존] 인벤토리 키
    PlayerInputComponent->BindAction("Inventory", IE_Pressed, this, &AMyCharacter::OnInventoryKeyPressed);

    // [추가] 도끼 지급 치트키 (=)
    // Project Settings -> Input -> Action Mappings에 "GetAxe"가 등록되어 있어야 함
    PlayerInputComponent->BindAction("GetAxe", IE_Pressed, this, &AMyCharacter::GetAxeCheat);

    // [기존] 퀵슬롯 키
	PlayerInputComponent->BindAction("QuickSlot1", IE_Pressed, this, &AMyCharacter::OnQuickSlot1);
	PlayerInputComponent->BindAction("QuickSlot2", IE_Pressed, this, &AMyCharacter::OnQuickSlot2);
	PlayerInputComponent->BindAction("QuickSlot3", IE_Pressed, this, &AMyCharacter::OnQuickSlot3);
	PlayerInputComponent->BindAction("QuickSlot4", IE_Pressed, this, &AMyCharacter::OnQuickSlot4);
	PlayerInputComponent->BindAction("QuickSlot5", IE_Pressed, this, &AMyCharacter::OnQuickSlot5);
	PlayerInputComponent->BindAction("QuickSlot6", IE_Pressed, this, &AMyCharacter::OnQuickSlot6);
	PlayerInputComponent->BindAction("QuickSlot7", IE_Pressed, this, &AMyCharacter::OnQuickSlot7);
	PlayerInputComponent->BindAction("QuickSlot8", IE_Pressed, this, &AMyCharacter::OnQuickSlot8);
}

void AMyCharacter::UpdateWeaponVisuals()
{
	// 1. 블루프린트에서 연결한 메쉬가 없으면 중단
	if (!WeaponMeshComp) return;

	// 2. 현재 선택된 슬롯 번호가 유효한지 체크
	if (Inventory.IsValidIndex(CurrentSelectedSlotIndex))
	{
		// 3. 선택된 아이템이 "Axe" 인지 확인
		if (Inventory[CurrentSelectedSlotIndex].ItemID == FName("Axe"))
		{
			WeaponMeshComp->SetVisibility(true); // 도끼면 보이게!
			return;
		}
	}

	// 4. 도끼가 아니거나 빈 슬롯이면 숨김
	WeaponMeshComp->SetVisibility(false);
}