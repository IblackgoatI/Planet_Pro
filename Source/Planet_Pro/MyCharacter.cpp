#include "MyCharacter.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "MyGameInstance.h" // GameInstance 헤더 필수
#include "MainHUDWidget.h"

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

    // [1] 커스터마이징 컴포넌트 찾기 (이름 정확도 향상)
    UE_LOG(LogTemp, Warning, TEXT("🛠️ [MyChar] 커스터마이징 컴포넌트 스캔 시작..."));

    // 1. 스켈레탈 메시 (우주선) 찾기
    TArray<USkeletalMeshComponent*> SkelComps;
    GetComponents(SkelComps);

    for (USkeletalMeshComponent* Comp : SkelComps)
    {
        FString CompName = Comp->GetName();
        // "CharacterMesh0"(언리얼 기본)는 피하고, "test1"이나 "SpaceShip" 포함된 것만 찾음
        if (!CompName.Equals(TEXT("CharacterMesh0")) && 
           (CompName.Contains(TEXT("test1")) || CompName.Contains(TEXT("SpaceShip")) || CompName.Contains(TEXT("Mesh"))))
        {
            Comp_SpaceShip_Skel = Comp;
            
            int32 MatCount = Comp->GetNumMaterials();
            // DMI 생성 (0: 겉면)
            if(MatCount > 0) DMI_Ship_Shell = Comp_SpaceShip_Skel->CreateAndSetMaterialInstanceDynamic(0);
            // DMI 생성 (4: 소파)
            if(MatCount > 4) DMI_Ship_Sofa = Comp_SpaceShip_Skel->CreateAndSetMaterialInstanceDynamic(4);
                
            UE_LOG(LogTemp, Warning, TEXT("✅ [MyChar] 우주선(Skel) 확정: %s (슬롯:%d)"), *CompName, MatCount);
            break; 
        }
    }

    // 2. 스태틱 메시 (캐릭터) 찾기
    TArray<UStaticMeshComponent*> StaticComps;
    GetComponents(StaticComps);

    for (UStaticMeshComponent* Comp : StaticComps)
    {
        if (Comp->GetName().Equals(TEXT("StaticMesh")))
        {
            Comp_CharBody = Comp;
            if(Comp->GetNumMaterials() > 0)
                DMI_Body = Comp_CharBody->CreateAndSetMaterialInstanceDynamic(0);

            UE_LOG(LogTemp, Warning, TEXT("✅ [MyChar] 캐릭터(Body) 찾음: %s"), *Comp->GetName());
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
    if (!WeaponMeshComp) return;

    if (Inventory.IsValidIndex(CurrentSelectedSlotIndex) &&
        Inventory[CurrentSelectedSlotIndex].ItemID == FName("Axe"))
    {
        WeaponMeshComp->SetVisibility(true);
    }
    else
    {
        WeaponMeshComp->SetVisibility(false);
    }
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
void AMyCharacter::ApplyCustomizationFromGI()
{
    UMyGameInstance* GI = Cast<UMyGameInstance>(GetGameInstance());
    if (!GI) return;

    // 1. 캐릭터 몸통
    if (DMI_Body)
    {
        if (GI->BodyTextureList.IsValidIndex(GI->MyCustomData.BodyIndex))
            DMI_Body->SetTextureParameterValue("BodyTex", GI->BodyTextureList[GI->MyCustomData.BodyIndex]);
        if (GI->EyeTextureList.IsValidIndex(GI->MyCustomData.EyeIndex))
            DMI_Body->SetTextureParameterValue("EyeTex", GI->EyeTextureList[GI->MyCustomData.EyeIndex]);
        if (GI->MouthTextureList.IsValidIndex(GI->MyCustomData.MouthIndex))
            DMI_Body->SetTextureParameterValue("MouthTex", GI->MouthTextureList[GI->MyCustomData.MouthIndex]);
    }

    // 2. 우주선
    int32 ShipIdx = GI->MyCustomData.MachineIndex;
    if (GI->ShipTextureList.IsValidIndex(ShipIdx))
    {
        if (DMI_Ship_Shell) DMI_Ship_Shell->SetTextureParameterValue("ShipTex", GI->ShipTextureList[ShipIdx]);
    }
    
    if (GI->SofaTextureList.IsValidIndex(ShipIdx))
    {
        if (DMI_Ship_Sofa) DMI_Ship_Sofa->SetTextureParameterValue("SofaTex", GI->SofaTextureList[ShipIdx]);
    }
}