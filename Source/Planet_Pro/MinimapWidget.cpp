#include "MinimapWidget.h"
#include "Components/Image.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"

void UMinimapWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (Img_MapLayer)
	{
		MinimapMatInst = Img_MapLayer->GetDynamicMaterial();
	}
}

void UMinimapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	ACharacter* PlayerChar = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	if (!PlayerChar) return;

	FVector PlayerLoc = PlayerChar->GetActorLocation();
	
	// 화살표 회전 (Mesh 기준)
	if (Img_PlayerArrow)
	{
		float PlayerYaw = PlayerChar->GetMesh()->GetComponentRotation().Yaw;
		Img_PlayerArrow->SetRenderTransformAngle(PlayerYaw);
	}

	// 맵 이동 (좌표 변환 정석)
	if (MinimapMatInst)
	{
		// 카메라 Yaw가 0일 때:
		// World Y (좌우) = Texture U (가로) -> 부호 그대로 (+)
		// World X (앞뒤) = Texture V (세로) -> 부호 반대 (-)
		
		// 중심점(0.5)에서 플레이어 이동만큼 뺌 (맵을 반대로 밀어야 하니까)
		float UV_Horizontal = (PlayerLoc.Y / WorldMapSize); // Add 노드 쓸 때는 0.5 안 더함 (UV Offset 개념)
		float UV_Vertical = (-PlayerLoc.X / WorldMapSize);

		// 머티리얼에서 Add로 연결했으므로, 맵을 반대 방향으로 밀어줘야 캐릭터가 가는 것처럼 보임
		MinimapMatInst->SetScalarParameterValue(FName("Y_Pos"), UV_Horizontal);
		MinimapMatInst->SetScalarParameterValue(FName("X_Pos"), UV_Vertical);
	}
}