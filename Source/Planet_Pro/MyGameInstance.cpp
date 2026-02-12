#include "MyGameInstance.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "JsonObjectConverter.h"

// PlayFab 관련
#include "PlayFab.h"
#include "Core/PlayFabClientDataModels.h"
#include "Core/PlayFabClientAPI.h"
#include "PlayFabError.h"

// ==========================================================
// ★ [핵심] 게임 시작 시 자동 로그인 로직
// ==========================================================
void UMyGameInstance::Init()
{
    Super::Init();

    UE_LOG(LogTemp, Warning, TEXT("🚀 [MyGameInstance] Init: 자동 로그인을 시작합니다..."));
    
    // 즉시 로그인 및 데이터 로드 함수 호출
    LoginAndLoadData();
}

// ==========================================================
// 1. 인벤토리 시스템
// ==========================================================
void UMyGameInstance::AddOrUpdateItem(FName InItemID, int32 InAmount)
{
    FItemData* FoundItem = MyInventory.FindByPredicate([&](const FItemData& Item){ return Item.ItemID == InItemID; });
    if (FoundItem) FoundItem->Amount += InAmount;
    else
    {
        FItemData NewItem;
        NewItem.ItemID = InItemID;
        NewItem.Amount = InAmount;
        MyInventory.Add(NewItem);
    }
}

FString UMyGameInstance::GetInventoryAsJsonString()
{
    TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject());
    TArray<TSharedPtr<FJsonValue>> ItemArray;

    for (const FItemData& Item : MyInventory)
    {
        TSharedPtr<FJsonObject> ItemObject = MakeShareable(new FJsonObject());
        ItemObject->SetStringField("ID", Item.ItemID.ToString()); // 저장할 땐 "ID"로 통일 (호환성)
        ItemObject->SetNumberField("Amount", Item.Amount);
        ItemArray.Add(MakeShareable(new FJsonValueObject(ItemObject)));
    }
    RootObject->SetArrayField("Items", ItemArray);

    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
    return OutputString;
}

TMap<FString, FString> UMyGameInstance::GetInventoryMapForPlayFab()
{
    TMap<FString, FString> DataMap;
    DataMap.Add(TEXT("Inventory"), GetInventoryAsJsonString());
    return DataMap;
}

void UMyGameInstance::SaveInventoryToPlayFab_CPP()
{
    FString JsonString = GetInventoryAsJsonString();
    
    PlayFab::ClientModels::FUpdateUserDataRequest Request;
    Request.Data.Add(TEXT("Inventory"), JsonString);

    auto ClientAPI = IPlayFabModuleInterface::Get().GetClientAPI();
    if (!ClientAPI.IsValid()) return;

    ClientAPI->UpdateUserData(Request,
        PlayFab::UPlayFabClientAPI::FUpdateUserDataDelegate::CreateLambda(
            [](const PlayFab::ClientModels::FUpdateUserDataResult& Result)
            {
                if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("C++: Inventory Save Success!"));
            }
        ),
        PlayFab::FPlayFabErrorDelegate::CreateLambda(
            [](const PlayFab::FPlayFabCppError& Error) 
            {
                if(GEngine) 
                {
                    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, 
                        FString::Printf(TEXT("Inventory Save Failed! %s"), *Error.ErrorMessage));
                }
            }
        )
    );
}

// ==========================================================
// 2. 시간 저장/로드 시스템
// ==========================================================
void UMyGameInstance::SaveSkyTime(float CurrentTime)
{
    auto ClientAPI = IPlayFabModuleInterface::Get().GetClientAPI();
    if (!ClientAPI.IsValid()) return;

    PlayFab::ClientModels::FUpdateUserDataRequest Request;
    FString TimeString = FString::SanitizeFloat(CurrentTime);
    Request.Data.Add(TEXT("SkyTime"), TimeString);

    ClientAPI->UpdateUserData(
        Request,
        PlayFab::UPlayFabClientAPI::FUpdateUserDataDelegate::CreateUObject(this, &UMyGameInstance::OnSaveTimeSuccess),
        PlayFab::FPlayFabErrorDelegate::CreateUObject(this, &UMyGameInstance::OnTimeError)
    );
}

void UMyGameInstance::LoadSkyTime()
{
    auto ClientAPI = IPlayFabModuleInterface::Get().GetClientAPI();
    if (!ClientAPI.IsValid()) return;

    PlayFab::ClientModels::FGetUserDataRequest Request;
    Request.Keys.Add(TEXT("SkyTime"));

    ClientAPI->GetUserData(
        Request,
        PlayFab::UPlayFabClientAPI::FGetUserDataDelegate::CreateUObject(this, &UMyGameInstance::OnLoadTimeSuccess),
        PlayFab::FPlayFabErrorDelegate::CreateUObject(this, &UMyGameInstance::OnTimeError)
    );
}

void UMyGameInstance::OnSaveTimeSuccess(const PlayFab::ClientModels::FUpdateUserDataResult& Result)
{
    if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("✅ [PlayFab] 시간 저장 성공!"));
    if (OnSaveSuccess.IsBound()) OnSaveSuccess.Broadcast();
}

void UMyGameInstance::OnLoadTimeSuccess(const PlayFab::ClientModels::FGetUserDataResult& Result)
{
    if (Result.Data.Contains(TEXT("SkyTime")))
    {
        FString TimeString = Result.Data[TEXT("SkyTime")].Value;
        SavedSkyTime = FCString::Atof(*TimeString);
        
        // 0.0 시간 방지 (하늘 멈춤 해결)
        if (SavedSkyTime <= 0.001f) SavedSkyTime = 8.0f;

        UE_LOG(LogTemp, Warning, TEXT("📥 [PlayFab] 시간 로드 완료: %f"), SavedSkyTime);
        if (OnSkyTimeLoaded.IsBound()) OnSkyTimeLoaded.Broadcast(SavedSkyTime);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ 저장된 시간이 없습니다."));
        SavedSkyTime = -1.0f; 
    }
}

void UMyGameInstance::OnTimeError(const PlayFab::FPlayFabCppError& ErrorResult)
{
    UE_LOG(LogTemp, Error, TEXT("❌ [PlayFab] Error: %s"), *ErrorResult.ErrorMessage);
}

// ==========================================================
// 3. [추가됨] 통합 저장 및 커스터마이징 구현부
// ==========================================================

void UMyGameInstance::LoginAndLoadData()
{
    auto ClientAPI = IPlayFabModuleInterface::Get().GetClientAPI();
    
    PlayFab::ClientModels::FLoginWithCustomIDRequest Request;
    Request.CustomId = FPlatformProcess::ComputerName();
    Request.CreateAccount = true;

    ClientAPI->LoginWithCustomID(Request,
        PlayFab::UPlayFabClientAPI::FLoginWithCustomIDDelegate::CreateUObject(this, &UMyGameInstance::OnLoginSuccess),
        PlayFab::FPlayFabErrorDelegate::CreateUObject(this, &UMyGameInstance::OnLoginFailure)
    );
}

void UMyGameInstance::OnLoginSuccess(const PlayFab::ClientModels::FLoginResult& Result)
{
    UE_LOG(LogTemp, Warning, TEXT("✅ [PlayFab] 로그인 성공! 통합 데이터를 불러옵니다."));

    auto ClientAPI = IPlayFabModuleInterface::Get().GetClientAPI();
    PlayFab::ClientModels::FGetUserDataRequest Request;
    // 인벤토리, 시간, 커스터마이징 데이터를 모두 요청
    Request.Keys = { TEXT("Inventory"), TEXT("SkyTime"), TEXT("CustomData") };

    ClientAPI->GetUserData(Request,
        PlayFab::UPlayFabClientAPI::FGetUserDataDelegate::CreateUObject(this, &UMyGameInstance::OnLoadDataSuccess),
        PlayFab::FPlayFabErrorDelegate::CreateUObject(this, &UMyGameInstance::OnLoginFailure)
    );
}

void UMyGameInstance::OnLoginFailure(const PlayFab::FPlayFabCppError& ErrorResult)
{
    UE_LOG(LogTemp, Error, TEXT("❌ [PlayFab] 로그인/로드 실패: %s"), *ErrorResult.ErrorMessage);
}

void UMyGameInstance::OnLoadDataSuccess(const PlayFab::ClientModels::FGetUserDataResult& Result)
{
    // 1. 인벤토리 로드
    if (Result.Data.Contains(TEXT("Inventory")))
    {
        JsonToInventory(Result.Data[TEXT("Inventory")].Value);
        UE_LOG(LogTemp, Warning, TEXT("   -> 인벤토리 로드됨 (%d개)"), MyInventory.Num());
    }

    // 2. 시간 로드
    if (Result.Data.Contains(TEXT("SkyTime")))
    {
        SavedSkyTime = FCString::Atof(*Result.Data[TEXT("SkyTime")].Value);
        if (SavedSkyTime <= 0.001f) SavedSkyTime = 8.0f; // 0.0 방지
        if (OnSkyTimeLoaded.IsBound()) OnSkyTimeLoaded.Broadcast(SavedSkyTime);
        UE_LOG(LogTemp, Warning, TEXT("   -> 시간 로드됨: %f"), SavedSkyTime);
    }

    // 3. 커스터마이징 로드
    if (Result.Data.Contains(TEXT("CustomData")))
    {
        JsonToCustomData(Result.Data[TEXT("CustomData")].Value);
        UE_LOG(LogTemp, Warning, TEXT("   -> 커스터마이징 로드됨 (Body:%d)"), MyCustomData.BodyIndex);
    }
    // ★★★ [제발 이 줄 추가] 이게 없어서 캐릭터가 데이터를 안 온 걸로 착각하는 겁니다! ★★★
    bIsDataLoaded = true;
    
    UE_LOG(LogTemp, Warning, TEXT("📥 [PlayFab] 통합 데이터 로드 최종 완료!"));
    
    if (OnDataLoadSuccess.IsBound())
    {
        OnDataLoadSuccess.Broadcast();
    }
}

void UMyGameInstance::SaveAllData()
{
    auto ClientAPI = IPlayFabModuleInterface::Get().GetClientAPI();
    if (!ClientAPI.IsValid()) return;

    PlayFab::ClientModels::FUpdateUserDataRequest Request;

    // 1. 인벤토리 추가
    Request.Data.Add(TEXT("Inventory"), GetInventoryAsJsonString());

    // 2. 시간 추가
    if (SavedSkyTime >= 0.0f)
    {
        Request.Data.Add(TEXT("SkyTime"), FString::SanitizeFloat(SavedSkyTime));
    }

    // 3. 커스터마이징 추가
    Request.Data.Add(TEXT("CustomData"), CustomDataToJson());

    // 통합 전송
    ClientAPI->UpdateUserData(Request,
        PlayFab::UPlayFabClientAPI::FUpdateUserDataDelegate::CreateUObject(this, &UMyGameInstance::OnSaveDataSuccess),
        PlayFab::FPlayFabErrorDelegate::CreateUObject(this, &UMyGameInstance::OnLoginFailure)
    );
}

void UMyGameInstance::OnSaveDataSuccess(const PlayFab::ClientModels::FUpdateUserDataResult& Result)
{
    UE_LOG(LogTemp, Log, TEXT("✅ [PlayFab] 통합 저장 성공!"));
    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, TEXT("Data Saved!"));

    if (OnSaveSuccess.IsBound())
    {
        OnSaveSuccess.Broadcast();
    }
}

// [헬퍼] CustomData -> JSON
FString UMyGameInstance::CustomDataToJson()
{
    FString OutputString;
    FJsonObjectConverter::UStructToJsonObjectString(MyCustomData, OutputString);
    return OutputString;
}

// [헬퍼] JSON -> CustomData
void UMyGameInstance::JsonToCustomData(const FString& JsonString)
{
    FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &MyCustomData);
}

// [헬퍼] JSON -> Inventory (로드용)
void UMyGameInstance::JsonToInventory(const FString& JsonString)
{
    TSharedPtr<FJsonObject> RootObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

    if (FJsonSerializer::Deserialize(Reader, RootObject))
    {
        MyInventory.Empty();
        const TArray<TSharedPtr<FJsonValue>>* ItemArray;
        if (RootObject->TryGetArrayField(TEXT("Items"), ItemArray))
        {
            for (const auto& Val : *ItemArray)
            {
                TSharedPtr<FJsonObject> ItemObj = Val->AsObject();
                FItemData NewItem;
                
                // ★ [호환성] ID와 ItemID 둘 다 체크
                if (ItemObj->HasField("ItemID")) NewItem.ItemID = FName(*ItemObj->GetStringField(TEXT("ItemID")));
                else if (ItemObj->HasField("ID")) NewItem.ItemID = FName(*ItemObj->GetStringField(TEXT("ID")));
                else NewItem.ItemID = FName("None");

                NewItem.Amount = ItemObj->GetNumberField(TEXT("Amount"));
                MyInventory.Add(NewItem);
            }
        }
    }
}