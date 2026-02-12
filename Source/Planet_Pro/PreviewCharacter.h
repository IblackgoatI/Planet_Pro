#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "PreviewCharacter.generated.h"

UCLASS()
class PLANET_PRO_API APreviewCharacter : public AActor
{
    GENERATED_BODY()
    
public:    
    APreviewCharacter();

protected:
    virtual void BeginPlay() override;

public:
    
    UPROPERTY(BlueprintReadWrite, Category = "Material")
    UMaterialInstanceDynamic* DMI_Sofa_Skel;
    
    // [컴포넌트]
    UPROPERTY(BlueprintReadWrite, Category = "Components")
    UStaticMeshComponent* Target_CharBody; 

    UPROPERTY(BlueprintReadWrite, Category = "Components")
    USkeletalMeshComponent* Target_Machine; 

    UPROPERTY(BlueprintReadWrite, Category = "Components")
    UStaticMeshComponent* Target_SpaceShip;

    // [기능 함수]
    UFUNCTION(BlueprintCallable, Category = "Customization")
    void UpdateParts(int32 BodyIdx, int32 EyeIdx, int32 MouthIdx);

    UFUNCTION(BlueprintCallable, Category = "Customization")
    void UpdateMachine(int32 MachineIdx);

    UFUNCTION(BlueprintCallable, Category = "Customization")
    void SetViewMode(int32 Mode);

    // ★ [핵심] 이 함수가 꼭 UFUNCTION이어야 예약이 작동합니다!
    UFUNCTION()
    void OnGILoadComplete(); 

    // [머티리얼]
    UPROPERTY(BlueprintReadWrite, Category = "Material")
    UMaterialInstanceDynamic* DMI_Body;

    UPROPERTY(BlueprintReadWrite, Category = "Material")
    UMaterialInstanceDynamic* DMI_Machine;

    UPROPERTY(BlueprintReadWrite, Category = "Material")
    UMaterialInstanceDynamic* DMI_Shell; 
    
    UPROPERTY(BlueprintReadWrite, Category = "Material")
    UMaterialInstanceDynamic* DMI_Sofa;  
};