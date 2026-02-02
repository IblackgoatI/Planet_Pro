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

    UE_LOG(LogTemp, Warning, TEXT("🕵️‍♂️ [Preview] BeginPlay 시작!"));

    // =================================================================
    // 1. [StaticMesh] 찾기 (캐릭터 몸통, 스태틱 우주선)
    // =================================================================
    TArray<UStaticMeshComponent*> StaticComps;
    GetComponents(StaticComps);

    for (UStaticMeshComponent* Comp : StaticComps)
    {
        FString Name = Comp->GetName();
        int32 MatCount = Comp->GetNumMaterials();

        // 1-1. 캐릭터 몸통 (Mesh_Char)
        if (Name.Contains(TEXT("Mesh_Char")))
        {
            Target_CharBody = Comp;
            // ★ [수정] 안전장치: 0번 재질이 있을 때만 생성
            if (MatCount > 0) 
            {
                DMI_Body = Target_CharBody->CreateAndSetMaterialInstanceDynamic(0);
            }
        }
        // 1-2. 스태틱 우주선 (StaticMesh)
        else if (Name.Contains(TEXT("StaticMesh")) || Name.Equals(TEXT("StaticMesh")))
        {
            Target_SpaceShip = Comp;
            UE_LOG(LogTemp, Warning, TEXT("🚀 [Static] 스태틱 우주선 발견! (재질 개수: %d)"), MatCount);

            if (MatCount > 0) DMI_Shell = Target_SpaceShip->CreateAndSetMaterialInstanceDynamic(0);
            
            // 소파 (4번 인덱스)
            if (MatCount > 4) 
            {
                DMI_Sofa = Target_SpaceShip->CreateAndSetMaterialInstanceDynamic(4);
                UE_LOG(LogTemp, Warning, TEXT("   ✅ [Static] 소파 DMI 생성 성공!"));
            }
        }
    }

    // =================================================================
    // 2. [SkeletalMesh] 찾기 (움직이는 우주선, 주사기 등) - ★ 여기가 핵심!
    // =================================================================
    TArray<USkeletalMeshComponent*> SkelComps;
    GetComponents(SkelComps);

    for (USkeletalMeshComponent* Comp : SkelComps)
    {
        FString Name = Comp->GetName();
        int32 MatCount = Comp->GetNumMaterials();

        // 2-1. 움직이는 우주선 (SpaceShip 또는 Machine)
        if (Name.Contains(TEXT("SpaceShip")) || Name.Contains(TEXT("Machine")))
        {
            Target_Machine = Comp;
            UE_LOG(LogTemp, Warning, TEXT("🦴 [Skeletal] 움직이는 우주선 발견! (이름: %s, 재질 개수: %d)"), *Name, MatCount);

            // 0번: 겉면
            if (MatCount > 0) 
            {
                DMI_Machine = Comp->CreateAndSetMaterialInstanceDynamic(0);
            }

            // ★ [핵심] 4번: 소파 (움직이는 우주선도 4번에 소파가 있어야 함)
            if (MatCount > 4)
            {
                DMI_Sofa_Skel = Comp->CreateAndSetMaterialInstanceDynamic(4);
                UE_LOG(LogTemp, Warning, TEXT("   ✅ [Skeletal] 소파 DMI_Skel 생성 성공!"));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("   ❌ [Skeletal] 재질 개수 부족! (현재: %d < 필요: 5) -> 소파 변경 불가"), MatCount);
            }
        }
    }

    // =================================================================
    // 3. 데이터 로딩 대기 로직 (기존 유지)
    // =================================================================
    UMyGameInstance* GI = Cast<UMyGameInstance>(GetGameInstance());
    if (GI)
    {
        if (GI->bIsDataLoaded)
        {
            UE_LOG(LogTemp, Warning, TEXT("⚡ [Preview] 데이터 이미 있음! 즉시 적용."));
            OnGILoadComplete();
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("⏳ [Preview] 데이터 로딩 중... 예약 걸기!"));
            GI->OnDataLoadSuccess.RemoveDynamic(this, &APreviewCharacter::OnGILoadComplete);
            GI->OnDataLoadSuccess.AddDynamic(this, &APreviewCharacter::OnGILoadComplete);
        }
    }
}
void APreviewCharacter::OnGILoadComplete()
{
    UMyGameInstance* GI = Cast<UMyGameInstance>(GetGameInstance());
    if (!GI) return;

    UE_LOG(LogTemp, Warning, TEXT("📬 [Preview] 데이터 도착! 적용 시작. (Body:%d, Ship:%d)"), 
        GI->MyCustomData.BodyIndex, GI->MyCustomData.MachineIndex);

    // ★ 여기서 실제로 옷을 입힙니다.
    UpdateParts(GI->MyCustomData.BodyIndex, GI->MyCustomData.EyeIndex, GI->MyCustomData.MouthIndex);
    UpdateMachine(GI->MyCustomData.MachineIndex);
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
        
        if (DMI_Sofa_Skel)
        {
            DMI_Sofa_Skel->SetTextureParameterValue(FName("SofaTex"), SofaTex);
            UE_LOG(LogTemp, Warning, TEXT("🛋️ [Skeletal] 움직이는 우주선 소파도 변경 완료!"));
        }
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