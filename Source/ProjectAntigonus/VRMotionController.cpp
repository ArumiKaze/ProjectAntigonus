#include "VRMotionController.h"
#include "MotionControllerComponent.h"
#include "SteamVRChaperoneComponent.h"
#include "Runtime/Engine/Classes/Components/StaticMeshComponent.h"
#include "Runtime/Engine/Classes/Kismet/KismetMathLibrary.h"
#include "Runtime/Engine/Classes/Components/SkeletalMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"
#include "Runtime/Engine/Classes/Haptics/HapticFeedbackEffect_Base.h"

AVRMotionController::AVRMotionController()
	:AVRMotionController(EControllerHand::Left)
{
}
AVRMotionController::AVRMotionController(EControllerHand hand)
{
	m_hand = hand;

	b_isroomscale = false;

	PrimaryActorTick.bCanEverTick = true;

	Scene = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));
	
	//Load Teleport Cylinder meshes and materials
	TeleportCylinder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TeleportCylinder"));
	TeleportCylinder->SetupAttachment(Scene);
	Ring = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ring"));
	Ring->SetupAttachment(TeleportCylinder);
	Arrow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Arrow"));
	Arrow->SetupAttachment(TeleportCylinder);
	RoomScaleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RoomScaleMesh"));
	RoomScaleMesh->SetupAttachment(Arrow);

	MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MotionController"));
	MotionController->SetupAttachment(Scene);
	MotionController->SetTrackingSource(EControllerHand::Left);
	if (hand == EControllerHand::Left)
	{
		MotionController->MotionSource = FName(TEXT("Left"));
	}
	else
	{
		MotionController->MotionSource = FName(TEXT("Right"));
	}

	HandMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HandMesh"));
	HandMesh->SetupAttachment(MotionController);

	SteamVRChaperone = CreateDefaultSubobject<USteamVRChaperoneComponent>(TEXT("SteamVRChaperone"));

	static ConstructorHelpers::FObjectFinder<UHapticFeedbackEffect_Base> hapticcurve(TEXT("HapticFeedbackEffect_Curve'/Game/VirtualReality/Mannequin/Character/HapticCurve_MotionController.HapticCurve_MotionController'"));
	if (hapticcurve.Object) {
		haptic_motioncontroller = hapticcurve.Object;
	}
}

void AVRMotionController::SetupRoomScaleOutline()
{
	FVector OutRectCenter{ 0.0f };
	FRotator OutRectRotation{ 0.0f };
	float OutSideLengthX{ 0.0f };
	float OutSideLengthY{ 0.0f };
	UKismetMathLibrary::MinimumAreaRectangle(this, SteamVRChaperone->GetBounds(), FVector(0.0f, 0.0f, 0.0f), OutRectCenter, OutRectRotation, OutSideLengthX, OutSideLengthY, false);

	//Measure Chaperone, default to 100x100 if roomscale is not used
	b_isroomscale = UKismetMathLibrary::BooleanNAND(UKismetMathLibrary::NearlyEqual_FloatFloat(OutSideLengthX, 100.0f, 0.01f), UKismetMathLibrary::NearlyEqual_FloatFloat(OutSideLengthY, 100.0f, 0.01f));
	if (b_isroomscale)
	{
		//Setup room-scale mesh (1x1x1 units in size by default) to the size of the room-scale dimensions
		float chaperonemeshheight{ 70.0f };
		RoomScaleMesh->SetWorldScale3D(FVector(OutSideLengthX, OutSideLengthY, chaperonemeshheight));
		RoomScaleMesh->SetRelativeRotation(OutRectRotation);
	}
}

void AVRMotionController::BeginPlay()
{
	Super::BeginPlay();
	
	SetupRoomScaleOutline();

	//Hide teleport cylinder until activation
	TeleportCylinder->SetVisibility(false, true);
	RoomScaleMesh->SetVisibility(false, true);

	//Invert scale on hand mesh to create left hand
	if (m_hand == EControllerHand::Left)
	{
		HandMesh->SetWorldScale3D(FVector(1.0f, 1.0f, -1.0f));
	}
}

void AVRMotionController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AVRMotionController::RumbleController(float intensity)
{
	UGameplayStatics::GetPlayerController(GetWorld(), 0)->PlayHapticEffect(haptic_motioncontroller, m_hand, intensity, false);
}