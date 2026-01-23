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
    // =================================================================
    // [1] 컴포넌트 (이름 복구 완료)
    // =================================================================
    
    // 캐릭터 (그대로 유지)
    UPROPERTY(BlueprintReadWrite, Category = "Components")
    UStaticMeshComponent* Target_CharBody; 

    // ★ [복구] 원래 쓰시던 스켈레탈 메시 이름
    UPROPERTY(BlueprintReadWrite, Category = "Components")
    USkeletalMeshComponent* Target_Machine; 

    // [추가] 우주선 뚜껑 (StaticMesh) - 이건 새로 필요한 거라 둠
    UPROPERTY(BlueprintReadWrite, Category = "Components")
    UStaticMeshComponent* Target_SpaceShip;


    // =================================================================
    // [2] 기능 함수
    // =================================================================
    UFUNCTION(BlueprintCallable, Category = "Customization")
    void UpdateParts(int32 BodyIdx, int32 EyeIdx, int32 MouthIdx);

    UFUNCTION(BlueprintCallable, Category = "Customization")
    void UpdateMachine(int32 MachineIdx);

    UFUNCTION(BlueprintCallable, Category = "Customization")
    void SetViewMode(int32 Mode);


    // =================================================================
    // [3] 다이내믹 머티리얼 (이름 복구 완료)
    // =================================================================
    
    // 캐릭터용
    UPROPERTY(BlueprintReadWrite, Category = "Material")
    UMaterialInstanceDynamic* DMI_Body;

    // ★ [복구] 원래 쓰시던 변수 이름 (블루프린트 오류 해결용)
    // -> 이제 이게 '다리/몸체(Skeletal)'의 0번 슬롯을 담당합니다.
    UPROPERTY(BlueprintReadWrite, Category = "Material")
    UMaterialInstanceDynamic* DMI_Machine;

    // [추가] 우주선 뚜껑용 (StaticMesh 0번)
    UPROPERTY(BlueprintReadWrite, Category = "Material")
    UMaterialInstanceDynamic* DMI_Shell; 
    
    // [추가] 우주선 소파용 (StaticMesh 4번)
    UPROPERTY(BlueprintReadWrite, Category = "Material")
    UMaterialInstanceDynamic* DMI_Sofa;  
};