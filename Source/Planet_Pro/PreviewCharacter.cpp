#include "PreviewCharacter.h"
#include "MyGameInstance.h"
#include "Kismet/GameplayStatics.h"

APreviewCharacter::APreviewCharacter()
{
    PrimaryActorTick.bCanEverTick = false;
}

void APreviewCharacter::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("🕵️‍♂️ [추적 시작] BeginPlay가 호출되었습니다!"));

    // =================================================================
    // 1. 스태틱 메시 찾기 (캐릭터 & 뚜껑)
    // =================================================================
    TArray<UStaticMeshComponent*> StaticComps;
    GetComponents(StaticComps);

    for (UStaticMeshComponent* Comp : StaticComps)
    {
        FString Name = Comp->GetName();
        int32 MatCount = Comp->GetNumMaterials(); // 재질이 몇 칸인지 확인

        UE_LOG(LogTemp, Warning, TEXT("🔎 검색된 스태틱 메시: %s (재질 슬롯 개수: %d)"), *Name, MatCount);

        // 1) 캐릭터 (Bean) 찾기
        if (Name.Contains(TEXT("Mesh_Char")))
        {
            Target_CharBody = Comp;
            
            // ★ 로그: 0번 시도한다고 알림
            UE_LOG(LogTemp, Warning, TEXT("👉 [시도] Mesh_Char의 0번 슬롯 DMI 생성 시도..."));
            
            if (MatCount > 0)
            {
                DMI_Body = Target_CharBody->CreateAndSetMaterialInstanceDynamic(0);
                if(DMI_Body) UE_LOG(LogTemp, Warning, TEXT("✅ [성공] Mesh_Char DMI 생성 완료!"));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("❌ [실패] Mesh_Char에 머티리얼이 하나도 없습니다! 블루프린트에서 머티리얼을 꽂아주세요."));
            }
        }
        // 2) 우주선 뚜껑 (cu) 찾기
        else if (Name.Equals(TEXT("StaticMesh")) || Name.Contains(TEXT("StaticMesh")))
        {
            Target_SpaceShip = Comp;
            
            // 0번 시도
            if (MatCount > 0)
            {
                DMI_Shell = Target_SpaceShip->CreateAndSetMaterialInstanceDynamic(0);
                UE_LOG(LogTemp, Warning, TEXT("✅ [성공] 뚜껑(StaticMesh) 0번 DMI 생성"));
            }

            // 4번 시도
            if (MatCount > 4)
            {
                DMI_Sofa = Target_SpaceShip->CreateAndSetMaterialInstanceDynamic(4);
                UE_LOG(LogTemp, Warning, TEXT("✅ [성공] 뚜껑(StaticMesh) 4번 DMI 생성"));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("⚠️ [주의] 뚜껑(StaticMesh)에 4번 슬롯이 없습니다! 현재 개수: %d개. (소파 색 변경 안됨)"), MatCount);
            }
        }
    }

    // =================================================================
    // 2. 스켈레탈 메시 찾기 (다리/몸체)
    // =================================================================
    TArray<USkeletalMeshComponent*> SkeletalComps;
    GetComponents(SkeletalComps);

    for (USkeletalMeshComponent* Comp : SkeletalComps)
    {
        if (Comp->GetName().Contains(TEXT("Mesh_Machine")))
        {
            Target_Machine = Comp; // 변수 이름 복구됨
            int32 SkelMatCount = Target_Machine->GetNumMaterials();

            UE_LOG(LogTemp, Warning, TEXT("🔎 검색된 스켈레탈 메시: %s (재질 슬롯 개수: %d)"), *Comp->GetName(), SkelMatCount);

            // 0번 시도
            if (SkelMatCount > 0)
            {
                DMI_Machine = Target_Machine->CreateAndSetMaterialInstanceDynamic(0); // 변수 이름 복구됨
                UE_LOG(LogTemp, Warning, TEXT("✅ [성공] 다리(Mesh_Machine) 0번 DMI 생성"));
            }
            break;
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("🕵️‍♂️ [추적 종료] BeginPlay 끝"));
}

void APreviewCharacter::UpdateParts(int32 BodyIdx, int32 EyeIdx, int32 MouthIdx)
{
    // 1. DMI 체크
    if (!DMI_Body)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ [UpdateParts] 실패: DMI_Body가 없습니다! (BeginPlay 확인 필요)"));
        return;
    }

    UMyGameInstance* GI = Cast<UMyGameInstance>(GetGameInstance());
    if (!GI) return;

    // 2. 몸통(Body) 변경 시도
    if (GI->BodyTextureList.IsValidIndex(BodyIdx))
    {
        UTexture2D* TargetTex = GI->BodyTextureList[BodyIdx];
        if (TargetTex)
        {
            // ★ 파라미터 이름 확인: "BodyTex"
            DMI_Body->SetTextureParameterValue(FName("BodyTex"), TargetTex);
            UE_LOG(LogTemp, Warning, TEXT("🎨 [Painting] 몸통 변경 시도! (파라미터: BodyTex, 이미지: %s)"), *TargetTex->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("❌ [Data] GameInstance의 BodyTextureList[%d]에 이미지가 없습니다! (None)"), BodyIdx);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ [Index] 몸통 인덱스 초과! (요청: %d, 배열길이: %d)"), BodyIdx, GI->BodyTextureList.Num());
    }

    // 3. 눈(Eye) 변경 시도
    if (GI->EyeTextureList.IsValidIndex(EyeIdx))
    {
        UTexture2D* TargetTex = GI->EyeTextureList[EyeIdx];
        if (TargetTex)
        {
            // ★ 파라미터 이름 확인: "EyeTex"
            DMI_Body->SetTextureParameterValue(FName("EyeTex"), TargetTex);
            UE_LOG(LogTemp, Warning, TEXT("🎨 [Painting] 눈 변경 시도! (파라미터: EyeTex, 이미지: %s)"), *TargetTex->GetName());
        }
    }

    // 4. 입(Mouth) 변경 시도
    if (GI->MouthTextureList.IsValidIndex(MouthIdx))
    {
        UTexture2D* TargetTex = GI->MouthTextureList[MouthIdx];
        if (TargetTex)
        {
            // ★ 파라미터 이름 확인: "MouthTex"
            DMI_Body->SetTextureParameterValue(FName("MouthTex"), TargetTex);
            UE_LOG(LogTemp, Warning, TEXT("🎨 [Painting] 입 변경 시도! (파라미터: MouthTex, 이미지: %s)"), *TargetTex->GetName());
        }
    }
}   

// ★ [핵심] 기능은 유지하되, 변수 이름은 원래대로
void APreviewCharacter::UpdateMachine(int32 MachineIdx)
{
    UMyGameInstance* GI = Cast<UMyGameInstance>(GetGameInstance());
    if (!GI) return;

    // 1. 겉면 색상 (ShipTex)
    if (GI->ShipTextureList.IsValidIndex(MachineIdx))
    {
        UTexture2D* ShellTex = GI->ShipTextureList[MachineIdx];

        // 뚜껑(Static) 변경
        if (DMI_Shell) DMI_Shell->SetTextureParameterValue(FName("ShipTex"), ShellTex);
        
        // ★ 다리(Skeletal) 변경 (원래 변수 사용)
        if (DMI_Machine) DMI_Machine->SetTextureParameterValue(FName("ShipTex"), ShellTex);
    }

    // 2. 소파 색상 (SofaTex)
    if (GI->SofaTextureList.IsValidIndex(MachineIdx))
    {
        UTexture2D* SofaTex = GI->SofaTextureList[MachineIdx];

        // 소파(Static 4번) 변경
        if (DMI_Sofa) DMI_Sofa->SetTextureParameterValue(FName("SofaTex"), SofaTex);
        
        // (만약 스켈레탈 메시에도 소파가 있다면 여기서 추가하면 됨)
    }
}

void APreviewCharacter::SetViewMode(int32 Mode)
{
    // ★ 변수 이름 Target_Machine으로 복구됨
    if (!Target_CharBody || !Target_Machine) return;

    switch (Mode)
    {
    case 0: // 캐릭터만
        Target_CharBody->SetVisibility(true);
        if(Target_SpaceShip) Target_SpaceShip->SetVisibility(false);
        Target_Machine->SetVisibility(false); // 복구된 이름
        break;

    case 1: // 우주선만
        Target_CharBody->SetVisibility(false);
        if(Target_SpaceShip) Target_SpaceShip->SetVisibility(true);
        Target_Machine->SetVisibility(true); // 복구된 이름
        break;

    case 2: // 전체
        Target_CharBody->SetVisibility(true);
        if(Target_SpaceShip) Target_SpaceShip->SetVisibility(true);
        Target_Machine->SetVisibility(true); // 복구된 이름
        break;
    }
}