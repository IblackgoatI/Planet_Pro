// PlanetPlayerController.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PlanetPlayerController.generated.h"

UCLASS()
class PLANET_PRO_API APlanetPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

public:
	// 1. 하늘 시간을 강제로 '꽂아넣는' 함수
	void ForceSkyUpdate();

	// 2. 준비 끝났으니 화면 밝히는 함수
	void FadeInScreen();
	
	UFUNCTION() 
	void OnSkyTimeLoadedReceived(float LoadedTime);
};