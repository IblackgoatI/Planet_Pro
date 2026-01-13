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
    if (!ClientAPI.IsValid()) return;

    ClientAPI->UpdateUserData(Request,
        PlayFab::UPlayFabClientAPI::FUpdateUserDataDelegate::CreateLambda(
            [](const PlayFab::ClientModels::FUpdateUserDataResult& Result)
            {
                if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("C++: Save Success!"));
            }
        ),
        // ★★★ [수정됨] 람다 인자 타입 변경 ★★★
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

// [1. 시간 저장 함수 수정]
void UMyGameInstance::SaveSkyTime(float CurrentTime)
{
    // 1. API 모듈 가져오기 (이게 제일 중요! Static 호출 대신 이걸 쓰세요)
    auto ClientAPI = IPlayFabModuleInterface::Get().GetClientAPI();
    
    // API가 유효하지 않으면 중단
    if (!ClientAPI.IsValid()) 
    {
        UE_LOG(LogTemp, Error, TEXT("PlayFab ClientAPI is invalid!"));
        return;
    }

    // 2. 요청서 만들기
    PlayFab::ClientModels::FUpdateUserDataRequest Request;
    FString TimeString = FString::SanitizeFloat(CurrentTime);
    Request.Data.Add(TEXT("SkyTime"), TimeString);

    // 3. API 호출 (인스턴스 방식)
    ClientAPI->UpdateUserData(
        Request,
        // 성공 델리게이트
        PlayFab::UPlayFabClientAPI::FUpdateUserDataDelegate::CreateUObject(this, &UMyGameInstance::OnSaveTimeSuccess),
        
        // ★★★ [수정 포인트] 에러 델리게이트 ★★★
        // PlayFab::UPlayFabClientAPI::FPlayFabErrorDelegate (X) -> 중간에 클래스 이름 뺴세요.
        // PlayFab::FPlayFabErrorDelegate (O) -> 이게 맞습니다.
        PlayFab::FPlayFabErrorDelegate::CreateUObject(this, &UMyGameInstance::OnTimeError)
    );
}

// [2. 시간 로드 함수 수정]
void UMyGameInstance::LoadSkyTime()
{
    // 1. API 모듈 가져오기
    auto ClientAPI = IPlayFabModuleInterface::Get().GetClientAPI();

    if (!ClientAPI.IsValid()) return;

    // 2. 요청서 만들기
    PlayFab::ClientModels::FGetUserDataRequest Request;
    Request.Keys.Add(TEXT("SkyTime"));

    // 3. API 호출
    ClientAPI->GetUserData(
        Request,
        PlayFab::UPlayFabClientAPI::FGetUserDataDelegate::CreateUObject(this, &UMyGameInstance::OnLoadTimeSuccess),
        
        // ★★★ [수정 포인트] 에러 델리게이트 ★★★
        PlayFab::FPlayFabErrorDelegate::CreateUObject(this, &UMyGameInstance::OnTimeError)
    );
}

void UMyGameInstance::OnLoadTimeSuccess(const PlayFab::ClientModels::FGetUserDataResult& Result)
{
    if (Result.Data.Contains(TEXT("SkyTime")))
    {
        FString TimeString = Result.Data[TEXT("SkyTime")].Value;
        SavedSkyTime = FCString::Atof(*TimeString);

        // ★ [핵심] "데이터 도착했어! 가져가서 써!" 하고 방송하기
        if (OnSkyTimeLoaded.IsBound())
        {
            OnSkyTimeLoaded.Broadcast(SavedSkyTime);
        }
        
        UE_LOG(LogTemp, Log, TEXT("방송 송출 완료: %f"), SavedSkyTime);
    }
    // 1. 데이터가 있는지 확인
    if (Result.Data.Contains(TEXT("SkyTime")))
    {
        // 2. String -> float 변환
        FString TimeString = Result.Data[TEXT("SkyTime")].Value;
        SavedSkyTime = FCString::Atof(*TimeString);

        UE_LOG(LogTemp, Log, TEXT("📥 [PlayFab] 시간 로드 완료: %f"), SavedSkyTime);
        if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, FString::Printf(TEXT("📥 시간 로드됨: %f"), SavedSkyTime));
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
    // 성공 로그 출력
    if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("✅ [PlayFab] 시간 저장 성공!"));
    UE_LOG(LogTemp, Log, TEXT("PlayFab Time Save Success!"));
}