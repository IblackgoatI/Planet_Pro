// MyGameInstance.cpp

#include "MyGameInstance.h"
#include "Json.h"
#include "JsonObjectConverter.h"

// PlayFab 관련
#include "PlayFab.h"
#include "Core/PlayFabClientDataModels.h"
#include "Core/PlayFabClientAPI.h"
#include "PlayFabError.h"

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
        ItemObject->SetStringField("ID", Item.ItemID.ToString());
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

// [인벤토리 저장]
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
                if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("C++: Save Success!"));
            }
        ),
        PlayFab::FPlayFabErrorDelegate::CreateLambda(
            [](const PlayFab::FPlayFabCppError& Error) 
            {
                if(GEngine) 
                {
                    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, 
                        FString::Printf(TEXT("Failed! %s"), *Error.ErrorMessage));
                }
            }
        )
    );
}

// [1. 시간 저장 함수]
void UMyGameInstance::SaveSkyTime(float CurrentTime)
{
    auto ClientAPI = IPlayFabModuleInterface::Get().GetClientAPI();
    
    if (!ClientAPI.IsValid()) 
    {
        UE_LOG(LogTemp, Error, TEXT("PlayFab ClientAPI is invalid!"));
        return;
    }

    PlayFab::ClientModels::FUpdateUserDataRequest Request;
    FString TimeString = FString::SanitizeFloat(CurrentTime);
    Request.Data.Add(TEXT("SkyTime"), TimeString);

    ClientAPI->UpdateUserData(
        Request,
        PlayFab::UPlayFabClientAPI::FUpdateUserDataDelegate::CreateUObject(this, &UMyGameInstance::OnSaveTimeSuccess),
        PlayFab::FPlayFabErrorDelegate::CreateUObject(this, &UMyGameInstance::OnTimeError)
    );
}

// [2. 시간 로드 함수]
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

// [로드 성공 처리 - 핵심 수정됨]
void UMyGameInstance::OnLoadTimeSuccess(const PlayFab::ClientModels::FGetUserDataResult& Result)
{
    // 데이터가 있는지 확인
    if (Result.Data.Contains(TEXT("SkyTime")))
    {
        FString TimeString = Result.Data[TEXT("SkyTime")].Value;
        SavedSkyTime = FCString::Atof(*TimeString);

        UE_LOG(LogTemp, Warning, TEXT("📥 [PlayFab] 시간 로드 완료: %f"), SavedSkyTime);
        if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, FString::Printf(TEXT("📥 시간 로드됨: %f"), SavedSkyTime));

        // ★ [핵심] "데이터 도착했어!" 하고 방송하기
        if (OnSkyTimeLoaded.IsBound())
        {
            OnSkyTimeLoaded.Broadcast(SavedSkyTime);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ 저장된 시간이 없습니다. (첫 실행 or 초기화)"));
        SavedSkyTime = -1.0f; // 데이터 없음 표시
    }
}

// 공통 에러 처리
void UMyGameInstance::OnTimeError(const PlayFab::FPlayFabCppError& ErrorResult)
{
    UE_LOG(LogTemp, Error, TEXT("❌ [PlayFab] Error: %s"), *ErrorResult.ErrorMessage);
    if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("❌ 실패: %s"), *ErrorResult.ErrorMessage));
}

void UMyGameInstance::OnSaveTimeSuccess(const PlayFab::ClientModels::FUpdateUserDataResult& Result)
{
    if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("✅ [PlayFab] 시간 저장 성공!"));
    UE_LOG(LogTemp, Log, TEXT("PlayFab Time Save Success!"));
}