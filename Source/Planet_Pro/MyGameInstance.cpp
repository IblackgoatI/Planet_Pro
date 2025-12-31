#include "MyGameInstance.h"
#include "Json.h"
#include "JsonObjectConverter.h"

// [핵심 수정] PlayFab 헤더 순서 및 파일명 변경
#include "PlayFab.h"
#include "Core/PlayFabClientDataModels.h"
#include "Core/PlayFabClientAPI.h"
#include "PlayFabError.h" // <--- FPlayFabError 정의가 여기 들어있습니다.

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

// [핵심] 저장 함수
void UMyGameInstance::SaveInventoryToPlayFab_CPP()
{
    // 1. JSON 문자열 만들기
    FString JsonString = GetInventoryAsJsonString();
    
    // 2. 요청서 만들기
    // (여기는 PlayFab::ClientModels:: 가 맞습니다. 이건 확실합니다.)
    PlayFab::ClientModels::FUpdateUserDataRequest Request;
    Request.Data.Add(TEXT("Inventory"), JsonString);

    // 3. API 모듈 가져오기
    auto ClientAPI = IPlayFabModuleInterface::Get().GetClientAPI();

    if (!ClientAPI.IsValid())
    {
        if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("PlayFab ClientAPI Invalid!"));
        return;
    }

    // 4. API 호출 (자동 타입 추론 사용)
    ClientAPI->UpdateUserData(Request,
        // [성공 시] 
        PlayFab::UPlayFabClientAPI::FUpdateUserDataDelegate::CreateLambda(
            [](const PlayFab::ClientModels::FUpdateUserDataResult& Result)
            {
                if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("C++: Save Success!"));
            }
        ),
        // [실패 시] - 여기가 문제였음!
        // PlayFab::FPlayFabError 대신 'const auto&'를 쓰면 알아서 찾습니다.
        PlayFab::FPlayFabErrorDelegate::CreateLambda(
            [](const auto& Error) 
            {
                if(GEngine) 
                {
                    // Error 객체 안의 ErrorMessage는 공통적으로 다 있습니다.
                    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, 
                        FString::Printf(TEXT("C++: Save Failed! %s"), *Error.ErrorMessage));
                }
            }
        )
    );
}