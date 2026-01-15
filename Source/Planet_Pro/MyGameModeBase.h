#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MyGameModeBase.generated.h"

UCLASS()
class PLANET_PRO_API AMyGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	// 게임(레벨) 시작 시 자동 실행되는 함수
	virtual void StartPlay() override;

private:
	// 시간 복구 함수
	void ApplySavedTime();
};