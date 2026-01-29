#include "MyCharacter.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "MyGameInstance.h" // GameInstance 헤더 필수
#include "MainHUDWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"

// PlayFab 헤더
#include "PlayFab.h"
#include "Core/PlayFabClientDataModels.h"
#include "Core/PlayFabClientAPI.h"

AMyCharacter::AMyCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
}

// =================================================================
// BeginPlay
// =================================================================
void AMyCharacter::BeginPlay()
{
    Super::BeginPlay();

    // 1. 현재 맵 이름 확인 (Lobby나 Game인지, Customizing인지)
    FString CurrentMapName = GetWorld()->GetMapName();
    bool bIsCustomizingMap = CurrentMapName.Contains(TEXT("Customizing"));

    UE_LOG(LogTemp, Warning, TEXT("🛠️ [MyChar] 맵 확인: %s (커마모드: %d)"), *CurrentMapName, bIsCustomizingMap);

    // =========================================================
    // [A] 컴포넌트 찾기 (스태틱/스켈레탈)
    // =========================================================
    TArray<UStaticMeshComponent*> StaticComps;
    GetComponents(StaticComps);

    for (UStaticMeshComponent* Comp : StaticComps)
    {
        FString CompName = Comp->GetName();

        // 1. 캐릭터 몸통 (공통)
        if (CompName.Equals(TEXT("Mesh_Char")) || (!bIsCustomizingMap && CompName.Equals(TEXT("StaticMesh"))))
        {
            Comp_CharBody = Comp;
            if(Comp->GetNumMaterials() > 0) 
                DMI_Body = Comp->CreateAndSetMaterialInstanceDynamic(0);
        }
        // 2. 우주선 (커마창 전용 - StaticMesh)
        else if (bIsCustomizingMap && CompName.Equals(TEXT("StaticMesh")))
        {
            Comp_SpaceShip_Static = Comp;
            int32 MatCount = Comp->GetNumMaterials();
            if (MatCount > 0) DMI_Ship_Shell = Comp->CreateAndSetMaterialInstanceDynamic(0);
            if (MatCount > 4) DMI_Ship_Sofa = Comp->CreateAndSetMaterialInstanceDynamic(4);
        }
    }

    // 인게임 전용 (Skeletal 우주선)
    if (!bIsCustomizingMap)
    {
        TArray<USkeletalMeshComponent*> SkelComps;
        GetComponents(SkelComps);
        for (USkeletalMeshComponent* Comp : SkelComps)
        {
            FString CompName = Comp->GetName();
            if (CompName.Contains(TEXT("test1")) || CompName.Contains(TEXT("SpaceShip")))
            {
                Comp_SpaceShip_Skel = Comp;
                int32 MatCount = Comp->GetNumMaterials();
                if(MatCount > 0) DMI_Ship_Shell = Comp->CreateAndSetMaterialInstanceDynamic(0);
                if(MatCount > 4) DMI_Ship_Sofa = Comp->CreateAndSetMaterialInstanceDynamic(4);
                break;
            }
        }
    }

    // =============================================================
    // ★ [핵심 수정] 타이밍 문제 해결 (비동기 로딩 대기)
    // =============================================================
    UMyGameInstance* GI = Cast<UMyGameInstance>(GetGameInstance());
    if (GI)
    {
        // 1. 만약 데이터가 이미 로드되어 있다면? -> 즉시 적용!
        if (GI->bIsDataLoaded)
        {
            UE_LOG(LogTemp, Warning, TEXT("⚡ [BeginPlay] 데이터가 이미 있습니다. 즉시 적용합니다!"));
            ApplyCustomizationFromGI();
        }
        // 2. 데이터가 아직 안 왔다면? -> "다 되면 불러줘"라고 예약!
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("⏳ [BeginPlay] 데이터 로딩 중... 도착하면 적용하도록 예약합니다."));
            
            // 혹시 모를 중복 방지를 위해 제거 후 추가
            GI->OnDataLoadSuccess.RemoveDynamic(this, &AMyCharacter::ApplyCustomizationFromGI);
            GI->OnDataLoadSuccess.AddDynamic(this, &AMyCharacter::ApplyCustomizationFromGI);
        }
    }


    // 3. 저장된 커마 적용
    ApplyCustomizationFromGI();

    // =============================================================
    // 무기 숨김
    // =============================================================
    if (WeaponMeshComp)
    {
        WeaponMeshComp->SetVisibility(false);
    }

    // =============================================================
    // HUD 생성 (로비 제외)
    // =============================================================
    FString MapName = GetWorld()->GetMapName();
    if (!MapName.Contains("Lobby") && MainHUDClass)
    {
        APlayerController* PC = Cast<APlayerController>(GetController());
        if (PC)
        {
            MainHUDInstance = CreateWidget<UMainHUDWidget>(PC, MainHUDClass);
            if (MainHUDInstance)
            {
                MainHUDInstance->AddToViewport();
                if (MainHUDInstance->InventoryWindow)
                    MainHUDInstance->InventoryWindow->SetVisibility(ESlateVisibility::Hidden);
            }
        }
    }

    // =============================================================
    // PlayFab 로그인 & 인벤토리 로드
    // =============================================================
    auto ClientAPI = IPlayFabModuleInterface::Get().GetClientAPI();

    if (ClientAPI.IsValid() && ClientAPI->IsClientLoggedIn())
    {
        LoadInventoryFromPlayFab();
    }
    else
    {
        // 로그인 안 되어 있으면 자동 로그인 시도
        PlayFab::ClientModels::FLoginWithCustomIDRequest Request;
        Request.CustomId = FPlatformProcess::ComputerName();
        Request.CreateAccount = true;

        ClientAPI->LoginWithCustomID(
            Request,
            PlayFab::UPlayFabClientAPI::FLoginWithCustomIDDelegate::CreateLambda(
                [this](const PlayFab::ClientModels::FLoginResult&)
                {
                    UE_LOG(LogTemp, Warning, TEXT("✅ [PlayFab] 자동 로그인 성공. 인벤토리 로드 시작."));
                    LoadInventoryFromPlayFab();
                }),
            PlayFab::FPlayFabErrorDelegate::CreateLambda(
                [](const PlayFab::FPlayFabCppError& Error)
                {
                    UE_LOG(LogTemp, Error, TEXT("❌ PlayFab 로그인 실패: %s"), *Error.ErrorMessage);
                })
        );
    }
}

// =================================================================
// Input Setup
// =================================================================
void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAction("Inventory", IE_Pressed, this, &AMyCharacter::OnInventoryKeyPressed);
    PlayerInputComponent->BindAction("GetAxe", IE_Pressed, this, &AMyCharacter::GetAxeCheat);

    PlayerInputComponent->BindAction("QuickSlot1", IE_Pressed, this, &AMyCharacter::OnQuickSlot1);
    PlayerInputComponent->BindAction("QuickSlot2", IE_Pressed, this, &AMyCharacter::OnQuickSlot2);
    PlayerInputComponent->BindAction("QuickSlot3", IE_Pressed, this, &AMyCharacter::OnQuickSlot3);
    PlayerInputComponent->BindAction("QuickSlot4", IE_Pressed, this, &AMyCharacter::OnQuickSlot4);
    PlayerInputComponent->BindAction("QuickSlot5", IE_Pressed, this, &AMyCharacter::OnQuickSlot5);
    PlayerInputComponent->BindAction("QuickSlot6", IE_Pressed, this, &AMyCharacter::OnQuickSlot6);
    PlayerInputComponent->BindAction("QuickSlot7", IE_Pressed, this, &AMyCharacter::OnQuickSlot7);
    PlayerInputComponent->BindAction("QuickSlot8", IE_Pressed, this, &AMyCharacter::OnQuickSlot8);
}

// =================================================================
// Inventory Logic
// =================================================================
void AMyCharacter::OnInventoryKeyPressed()
{
    if (MainHUDInstance)
    {
        MainHUDInstance->ToggleInventory();
        if (MainHUDInstance->bIsInventoryOpen)
            MainHUDInstance->RefreshInventory(Inventory);
    }
}

void AMyCharacter::AddInventoryItem(FName NewItemID, int32 NewAmount)
{
    if (NewAmount <= 0) return;

    // 1. 기존 아이템 합치기
    for (FPlanetItemInfo& Item : Inventory)
    {
        if (Item.ItemID == NewItemID)
        {
            Item.Amount += NewAmount;
            if (MainHUDInstance) MainHUDInstance->RefreshInventory(Inventory);
            SaveInventoryToPlayFab();
            return;
        }
    }

    // 2. 빈 슬롯 찾기
    for (int32 i = 0; i < Inventory.Num(); i++)
    {
        if (Inventory[i].ItemID == FName("None") || Inventory[i].Amount <= 0)
        {
            Inventory[i].ItemID = NewItemID;
            Inventory[i].Amount = NewAmount;
            if (MainHUDInstance) MainHUDInstance->RefreshInventory(Inventory);
            SaveInventoryToPlayFab();
            if (i == CurrentSelectedSlotIndex) UpdateWeaponVisuals();
            return; 
        }
    }

    // 3. 새 슬롯 추가
    FPlanetItemInfo NewItem;
    NewItem.ItemID = NewItemID;
    NewItem.Amount = NewAmount;
    Inventory.Add(NewItem);

    if (MainHUDInstance) MainHUDInstance->RefreshInventory(Inventory);
    SaveInventoryToPlayFab();
}

void AMyCharacter::SwapInventoryItems(int32 A, int32 B)
{
    if (A == B) return;
    if (A < 0 || B < 0) return;

    int32 MaxIndex = FMath::Max(A, B);
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

    Inventory.Swap(A, B);

    if (MainHUDInstance) MainHUDInstance->RefreshInventory(Inventory);
    SaveInventoryToPlayFab();
    UpdateWeaponVisuals();
}

// =================================================================
// PlayFab Save / Load
// =================================================================
void AMyCharacter::SaveInventoryToPlayFab()
{
    auto ClientAPI = IPlayFabModuleInterface::Get().GetClientAPI();
    if (!ClientAPI.IsValid() || !ClientAPI->IsClientLoggedIn()) return;

    TArray<TSharedPtr<FJsonValue>> JsonArray;
    for (const auto& Item : Inventory)
    {
        TSharedPtr<FJsonObject> Obj = MakeShareable(new FJsonObject);
        Obj->SetStringField("ItemID", Item.ItemID.ToString()); // 저장할 땐 "ItemID"로 통일
        Obj->SetNumberField("Amount", Item.Amount);
        JsonArray.Add(MakeShareable(new FJsonValueObject(Obj)));
    }

    TSharedPtr<FJsonObject> Root = MakeShareable(new FJsonObject);
    Root->SetArrayField("Items", JsonArray);

    FString Output;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);

    PlayFab::ClientModels::FUpdateUserDataRequest Req;
    Req.Data.Add("Inventory", Output);

    ClientAPI->UpdateUserData(Req,
        PlayFab::UPlayFabClientAPI::FUpdateUserDataDelegate::CreateUObject(this, &AMyCharacter::OnSaveSuccess),
        PlayFab::FPlayFabErrorDelegate::CreateUObject(this, &AMyCharacter::OnSaveError));
}

void AMyCharacter::LoadInventoryFromPlayFab()
{
    auto ClientAPI = IPlayFabModuleInterface::Get().GetClientAPI();
    if (!ClientAPI.IsValid()) return;

    PlayFab::ClientModels::FGetUserDataRequest Req;
    Req.Keys.Add("Inventory");

    ClientAPI->GetUserData(
        Req,
        PlayFab::UPlayFabClientAPI::FGetUserDataDelegate::CreateUObject(this, &AMyCharacter::OnLoadSuccess),
        PlayFab::FPlayFabErrorDelegate::CreateUObject(this, &AMyCharacter::OnLoadError)
    );
}

// ★ [핵심 수정] 안전한 JSON 파싱 (Null String 에러 해결)
void AMyCharacter::OnLoadSuccess(const PlayFab::ClientModels::FGetUserDataResult& Result)
{
    if (!Result.Data.Contains("Inventory")) 
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ [PlayFab] 저장된 인벤토리 없음."));
        return;
    }

    FString JsonString = Result.Data["Inventory"].Value;
    if (JsonString.IsEmpty()) return;

    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (FJsonSerializer::Deserialize(Reader, Root))
    {
        Inventory.Empty(); // 로드 전 초기화

        const TArray<TSharedPtr<FJsonValue>>* Items;
        if (Root->TryGetArrayField("Items", Items))
        {
            for (auto& V : *Items)
            {
                TSharedPtr<FJsonObject> Obj = V->AsObject();
                if (!Obj.IsValid()) continue;

                FPlanetItemInfo Item;
                
                // ★ [안전장치] ItemID가 있으면 읽고, 없으면 ID를 읽음
                if (Obj->HasField("ItemID"))
                {
                    Item.ItemID = FName(*Obj->GetStringField("ItemID"));
                }
                else if (Obj->HasField("ID"))
                {
                    Item.ItemID = FName(*Obj->GetStringField("ID"));
                }
                else
                {
                    Item.ItemID = FName("None");
                }

                // Amount 읽기
                if (Obj->HasField("Amount"))
                {
                    Item.Amount = Obj->GetNumberField("Amount");
                }
                else
                {
                    Item.Amount = 1;
                }

                // 유효한 아이템만 추가
                if (Item.ItemID != FName("None") && Item.Amount > 0)
                {
                    Inventory.Add(Item);
                }
            }
        }
        
        UE_LOG(LogTemp, Warning, TEXT("✅ [PlayFab] 인벤토리 로드 완료 (총 %d개)"), Inventory.Num());
    }

    if (MainHUDInstance) MainHUDInstance->RefreshInventory(Inventory);
    UpdateWeaponVisuals();
}

void AMyCharacter::OnLoadError(const PlayFab::FPlayFabCppError& Error)
{
    UE_LOG(LogTemp, Error, TEXT("❌ Load 실패: %s"), *Error.ErrorMessage);
}

void AMyCharacter::OnSaveSuccess(const PlayFab::ClientModels::FUpdateUserDataResult&) {}
void AMyCharacter::OnSaveError(const PlayFab::FPlayFabCppError& Error)
{
    UE_LOG(LogTemp, Error, TEXT("❌ Save 실패: %s"), *Error.ErrorMessage);
}

// =================================================================
// QuickSlot / Weapon
// =================================================================
void AMyCharacter::SelectQuickSlot(int32 Slot)
{
    CurrentSelectedSlotIndex = Slot;
    if (MainHUDInstance)
        MainHUDInstance->UpdateQuickSlotHighlight(Slot);
    UpdateWeaponVisuals();
}

void AMyCharacter::UpdateWeaponVisuals()
{
    // 1. 현재 선택된 아이템 확인
    if (!Inventory.IsValidIndex(CurrentSelectedSlotIndex)) return;

    FName CurrentItemID = Inventory[CurrentSelectedSlotIndex].ItemID;
    
    // 2. 상태 결정 (Enum 값 정하기)
    ECharacterWeaponState NewState = ECharacterWeaponState::Unarmed;

    if (CurrentItemID == FName("Axe"))
    {
        NewState = ECharacterWeaponState::Axe;
    }
    else if (CurrentItemID == FName("Wood"))
    {
        NewState = ECharacterWeaponState::Wood;
    }
    else if (CurrentItemID == FName("Berry") || CurrentItemID == FName("Berry_D"))
    {
        NewState = ECharacterWeaponState::Berry;
    }

    // 3. 상태 변수 업데이트 (애니메이션 BP가 이걸 보고 동작을 바꿈)
    CurrentWeaponState = NewState;

    // ★ 4. 블루프린트한테 명령 내리기! (여기서 시각적 처리를 넘깁니다)
    BP_UpdateEquippedItem(NewState); 
    
    // 로그 확인
    UE_LOG(LogTemp, Warning, TEXT("명령 전달함: 상태 %d"), (int32)NewState);
}

void AMyCharacter::OnQuickSlot1(){ SelectQuickSlot(0); }
void AMyCharacter::OnQuickSlot2(){ SelectQuickSlot(1); }
void AMyCharacter::OnQuickSlot3(){ SelectQuickSlot(2); }
void AMyCharacter::OnQuickSlot4(){ SelectQuickSlot(3); }
void AMyCharacter::OnQuickSlot5(){ SelectQuickSlot(4); }
void AMyCharacter::OnQuickSlot6(){ SelectQuickSlot(5); }
void AMyCharacter::OnQuickSlot7(){ SelectQuickSlot(6); }
void AMyCharacter::OnQuickSlot8(){ SelectQuickSlot(7); }

// =================================================================
// Cheat
// =================================================================
void AMyCharacter::GetAxeCheat()
{
    // 중복 체크
    for (const auto& Item : Inventory) { if (Item.ItemID == FName("Axe")) return; }

    // 공간 확보
    if (Inventory.Num() <= 0) Inventory.SetNum(1);

    // 0번 자리 이사 준비
    FPlanetItemInfo OldItem = Inventory[0];
    bool bHasOldItem = (OldItem.ItemID != FName("None") && OldItem.Amount > 0);

    // 도끼 지급
    Inventory[0].ItemID = FName("Axe");
    Inventory[0].Amount = 1;

    // 밀려난 아이템 재배치
    if (bHasOldItem) AddInventoryItem(OldItem.ItemID, OldItem.Amount);

    if (MainHUDInstance) MainHUDInstance->RefreshInventory(Inventory);
    SaveInventoryToPlayFab();
    
    if (CurrentSelectedSlotIndex == 0) UpdateWeaponVisuals();
}

// =================================================================
// Customization (GameInstance -> Character)
// =================================================================
// =============================================================
// ★ [수정] 보내주신 로그 강화 버전 (그대로 사용)
// =============================================================
void AMyCharacter::ApplyCustomizationFromGI()
{
    UMyGameInstance* GI = Cast<UMyGameInstance>(GetGameInstance());
    if (!GI) 
    {
        UE_LOG(LogTemp, Error, TEXT("❌ [ApplyCustomization] GameInstance를 찾을 수 없습니다!"));
        return;
    }

    // [0] 데이터 준비 상태 확인 로그
    if (!GI->bIsDataLoaded)
    {
        UE_LOG(LogTemp, Error, TEXT("⚠️ [ApplyCustomization] 주의: 데이터 로딩 완료 전 호출됨 (기본값 가능성 있음)"));
    }

    int32 BodyIdx = GI->MyCustomData.BodyIndex;
    int32 EyeIdx = GI->MyCustomData.EyeIndex;
    int32 MouthIdx = GI->MyCustomData.MouthIndex;
    int32 ShipIdx = GI->MyCustomData.MachineIndex;

    UE_LOG(LogTemp, Warning, TEXT("🔍 [ApplyCustomization] 적용 시도 -> Body: %d, Eye: %d, Mouth: %d, Ship: %d"), BodyIdx, EyeIdx, MouthIdx, ShipIdx);

    // [1] 캐릭터 몸통 적용
    if (DMI_Body)
    {
        // 1. 몸통 텍스처
        if (GI->BodyTextureList.IsValidIndex(BodyIdx))
        {
            UTexture2D* Tex = GI->BodyTextureList[BodyIdx];
            if (Tex)
            {
                DMI_Body->SetTextureParameterValue(TEXT("BodyTex"), Tex);
                UE_LOG(LogTemp, Log, TEXT("   ✅ [Body] BodyTex 적용 완료 (Index: %d)"), BodyIdx);
            }
            else UE_LOG(LogTemp, Error, TEXT("   ❌ [Body] 인덱스는 유효하지만 텍스처 파일이 비어있음(Null)! GI 확인 필요."));
        }
        else UE_LOG(LogTemp, Error, TEXT("   ❌ [Body] 인덱스 범위를 벗어남! (요청: %d, 전체개수: %d)"), BodyIdx, GI->BodyTextureList.Num());

        // 2. 눈 텍스처
        if (GI->EyeTextureList.IsValidIndex(EyeIdx))
            DMI_Body->SetTextureParameterValue(TEXT("EyeTex"), GI->EyeTextureList[EyeIdx]);
        
        // 3. 입 텍스처
        if (GI->MouthTextureList.IsValidIndex(MouthIdx))
            DMI_Body->SetTextureParameterValue(TEXT("MouthTex"), GI->MouthTextureList[MouthIdx]);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ [Body] DMI_Body가 없습니다! (BeginPlay에서 메시를 못 찾았거나 DMI 생성 실패)"));
    }
    
    // [2] 우주선 적용 (스태틱/스켈레탈 공통 DMI 사용)
    if (DMI_Ship_Shell) 
    {
        if (GI->ShipTextureList.IsValidIndex(ShipIdx))
        {
            UTexture2D* ShipTex = GI->ShipTextureList[ShipIdx];
            if (ShipTex)
            {
                DMI_Ship_Shell->SetTextureParameterValue(TEXT("ShipTex"), ShipTex);
                UE_LOG(LogTemp, Log, TEXT("   ✅ [Ship] ShipTex 적용 완료 (Index: %d)"), ShipIdx);
            }
            else UE_LOG(LogTemp, Error, TEXT("   ❌ [Ship] 텍스처 파일이 비어있음(Null)! GI 확인 필요."));
        }
        else UE_LOG(LogTemp, Error, TEXT("   ❌ [Ship] 인덱스 범위를 벗어남! (요청: %d, 전체개수: %d)"), ShipIdx, GI->ShipTextureList.Num());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ [Ship] DMI_Ship_Shell이 없습니다! (BeginPlay에서 우주선 메시 이름 확인 요망)"));
    }

    // [3] 우주선 소파 적용
    if (DMI_Ship_Sofa && GI->SofaTextureList.IsValidIndex(ShipIdx))
    {
        DMI_Ship_Sofa->SetTextureParameterValue(TEXT("SofaTex"), GI->SofaTextureList[ShipIdx]);
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🏁 [ApplyCustomization] 함수 종료"));
}

bool AMyCharacter::IsAxeEquipped()
{
    // 1. 현재 선택된 슬롯이 유효한지 확인
    if (Inventory.IsValidIndex(CurrentSelectedSlotIndex))
    {
        // 2. 해당 슬롯의 아이템 ID가 "Axe"인지 확인
        if (Inventory[CurrentSelectedSlotIndex].ItemID == FName("Axe"))
        {
            return true; // 도끼 맞음!
        }
    }

    return false; // 도끼 아님 (빈손이거나 다른 아이템)
}

bool AMyCharacter::ConsumeInventoryItem(FName TargetItemID, int32 AmountToConsume)
{
    // 1. 인벤토리 순회하며 아이템 찾기
    for (int32 i = 0; i < Inventory.Num(); i++)
    {
        // 아이템 ID가 일치하고, 개수가 충분한지 확인
        if (Inventory[i].ItemID == TargetItemID && Inventory[i].Amount >= AmountToConsume)
        {
            // 2. 개수 차감
            Inventory[i].Amount -= AmountToConsume;

            // 3. 개수가 0이 되면 아이템 슬롯 비우기 (None 처리)
            if (Inventory[i].Amount <= 0)
            {
                Inventory[i].ItemID = FName("None");
                Inventory[i].Amount = 0;
            }

            // 4. 변경 사항 반영 (UI 갱신 + PlayFab 저장)
            if (MainHUDInstance) 
            {
                MainHUDInstance->RefreshInventory(Inventory);
            }
            
            // 무기 슬롯이었다면 비주얼 업데이트
            if (i == CurrentSelectedSlotIndex) 
            {
                UpdateWeaponVisuals();
            }

            // ★ PlayFab에 저장 (서버 동기화)
           //SaveInventoryToPlayFab();
            // ★ 스마트 저장 요청 (3초 뒤 저장)
            RequestSmartSave(); 
            UE_LOG(LogTemp, Warning, TEXT("✅ [ConsumeItem] %s 아이템 %d개 소모 성공!"), *TargetItemID.ToString(), AmountToConsume);
            return true; // 성공!
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("❌ [ConsumeItem] %s 아이템이 부족하거나 없습니다."), *TargetItemID.ToString());
    return false; // 실패 (아이템 없음)
}

void AMyCharacter::RequestSmartSave()
{
    // 이미 타이머가 돌고 있다면 취소하고 다시 설정 (시간 연장)
    GetWorld()->GetTimerManager().ClearTimer(SaveTimerHandle);

    // 3초 뒤에 SaveInventoryToPlayFab 함수를 실행해라!
    GetWorld()->GetTimerManager().SetTimer(SaveTimerHandle, this, &AMyCharacter::SaveInventoryToPlayFab, 3.0f, false);

    UE_LOG(LogTemp, Log, TEXT("⏳ [SmartSave] 3초 뒤 저장 예약됨... (연타하면 시간 연장)"));
}