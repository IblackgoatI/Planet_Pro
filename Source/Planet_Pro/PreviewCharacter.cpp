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

    // 1. 컴포넌트 찾기 (이 부분은 잘 되므로 기존 유지)
    TArray<UStaticMeshComponent*> StaticComps;
    GetComponents(StaticComps);

    for (UStaticMeshComponent* Comp : StaticComps)
    {
        FString Name = Comp->GetName();
        int32 MatCount = Comp->GetNumMaterials();

        if (Name.Contains(TEXT("Mesh_Char")))
        {
            Target_CharBody = Comp;
            if (MatCount > 0) DMI_Body = Target_CharBody->CreateAndSetMaterialInstanceDynamic(0);
        }
        else if (Name.Equals(TEXT("StaticMesh")) || Name.Contains(TEXT("StaticMesh"))) // "cu"가 들어있는 컴포넌트
        {
            Target_SpaceShip = Comp;
            if (MatCount > 0) DMI_Shell = Target_SpaceShip->CreateAndSetMaterialInstanceDynamic(0); // M_BaseColor (0번)
            if (MatCount > 4) DMI_Sofa = Target_SpaceShip->CreateAndSetMaterialInstanceDynamic(4);  // M_Sofa (4번)
        }
    }
    
    // ... 스켈레탈 메시 찾는 코드 생략 (기존 유지) ...
    // 혹시 모르니 스켈레탈 메시 찾는 부분도 DMI_Machine 연결 잘 되어있는지 확인해주세요.

    // =================================================================
    // 2. 데이터 로딩 대기 로직 (수정됨)
    // =================================================================
    UMyGameInstance* GI = Cast<UMyGameInstance>(GetGameInstance());
    if (GI)
    {
        // 1. 이미 데이터가 있으면 -> 바로 적용
        if (GI->bIsDataLoaded)
        {
            UE_LOG(LogTemp, Warning, TEXT("⚡ [Preview] 데이터 이미 있음! (Body:%d, Ship:%d) 즉시 적용."), 
                GI->MyCustomData.BodyIndex, GI->MyCustomData.MachineIndex);
            OnGILoadComplete(); 
        }
        // 2. 데이터가 없으면 -> 예약
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("⏳ [Preview] 데이터 로딩 중... 예약 걸기!"));
            
            GI->OnDataLoadSuccess.RemoveDynamic(this, &APreviewCharacter::OnGILoadComplete);
            // ★ 헤더에 UFUNCTION() 없으면 여기서 터지거나 무시됨
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