#include "VRMotionController.h"
#include "MotionControllerComponent.h"
#include "SteamVRChaperoneComponent.h"
#include "Runtime/Engine/Classes/Components/StaticMeshComponent.h"
#include "Runtime/Engine/Classes/Kismet/KismetMathLibrary.h"
#include "Runtime/Engine/Classes/Components/SkeletalMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"
#include "Runtime/Engine/Classes/Haptics/HapticFeedbackEffect_Base.h"
#include "Runtime/Engine/Classes/Components/SphereComponent.h"

AVRMotionController::AVRMotionController()
	:AVRMotionController(EControllerHand::Left)
{
}
AVRMotionController::AVRMotionController(EControllerHand hand)
{
	//Attached actor object
	attachedactor = nullptr;

	//Grip enum state
	gripstate = E_GRIPSTATE::GRIP_OPEN;

	m_hand = hand;

	b_isroomscale = false;

	b_shouldgrip = false;

	PrimaryActorTick.bCanEverTick = true;

	scene = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));

	//Load hand
	motioncontroller = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MotionController"));
	motioncontroller->SetupAttachment(scene);
	motioncontroller->SetTrackingSource(EControllerHand::Left);
	if (hand == EControllerHand::Left)
	{
		motioncontroller->MotionSource = FName(TEXT("Left"));
	}
	else
	{
		motioncontroller->MotionSource = FName(TEXT("Right"));
	}
	handmesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HandMesh"));
	handmesh->SetupAttachment(motioncontroller);
	grabsphere = CreateDefaultSubobject<USphereComponent>(TEXT("GrabSphere"));
	grabsphere->SetupAttachment(handmesh);
	grabsphere->SetSphereRadius(10.0f);
	
	//Load Teleport Cylinder meshes and materials
	teleportcylinder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TeleportCylinder"));
	teleportcylinder->SetupAttachment(scene);
	ring = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ring"));
	ring->SetupAttachment(teleportcylinder);
	arrow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Arrow"));
	arrow->SetupAttachment(teleportcylinder);
	roomscalemesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RoomScaleMesh"));
	roomscalemesh->SetupAttachment(arrow);

	//Load SteamVR Chaperone
	steamvrchaperone = CreateDefaultSubobject<USteamVRChaperoneComponent>(TEXT("SteamVRChaperone"));

	//Load haptic feedback curve
	static ConstructorHelpers::FObjectFinder<UHapticFeedbackEffect_Base> hapticcurve(TEXT("HapticFeedbackEffect_Curve'/Game/VirtualReality/Mannequin/Character/HapticCurve_MotionController.HapticCurve_MotionController'"));
	if (hapticcurve.Object) {
		haptic_motioncontroller = hapticcurve.Object;
	}
}


void AVRMotionController::BeginPlay()
{
	Super::BeginPlay();
	
	SetupRoomScaleOutline();

	//Hide teleport cylinder until activation
	teleportcylinder->SetVisibility(false, true);
	roomscalemesh->SetVisibility(false, true);

	//Invert scale on hand mesh to create left hand
	if (m_hand == EControllerHand::Left)
	{
		handmesh->SetWorldScale3D(FVector(1.0f, 1.0f, -1.0f));
	}
}

void AVRMotionController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//Update animation of hand
	if ((attachedactor != nullptr) || b_shouldgrip)
	{
		gripstate = E_GRIPSTATE::GRIP_GRAB;
	}
	else
	{
		if (GetActorNearHand() != nullptr)
		{
			gripstate = E_GRIPSTATE::GRIP_CANGRAB;
		}
		else
		{
			if (b_shouldgrip)
			{
				gripstate = E_GRIPSTATE::GRIP_GRAB;
			}
			else
			{
				gripstate = E_GRIPSTATE::GRIP_OPEN;
			}
		}
	}
}

void AVRMotionController::SetupRoomScaleOutline()
{
	FVector OutRectCenter{ 0.0f };
	FRotator OutRectRotation{ 0.0f };
	float OutSideLengthX{ 0.0f };
	float OutSideLengthY{ 0.0f };
	UKismetMathLibrary::MinimumAreaRectangle(this, steamvrchaperone->GetBounds(), FVector(0.0f, 0.0f, 0.0f), OutRectCenter, OutRectRotation, OutSideLengthX, OutSideLengthY, false);

	//Measure Chaperone, default to 100x100 if roomscale is not used
	b_isroomscale = UKismetMathLibrary::BooleanNAND(UKismetMathLibrary::NearlyEqual_FloatFloat(OutSideLengthX, 100.0f, 0.01f), UKismetMathLibrary::NearlyEqual_FloatFloat(OutSideLengthY, 100.0f, 0.01f));
	if (b_isroomscale)
	{
		//Setup room-scale mesh (1x1x1 units in size by default) to the size of the room-scale dimensions
		float chaperonemeshheight{ 70.0f };
		roomscalemesh->SetWorldScale3D(FVector(OutSideLengthX, OutSideLengthY, chaperonemeshheight));
		roomscalemesh->SetRelativeRotation(OutRectRotation);
	}
}

void AVRMotionController::RumbleController(float intensity)
{
	UGameplayStatics::GetPlayerController(GetWorld(), 0)->PlayHapticEffect(haptic_motioncontroller, m_hand, intensity, false);
}

//---Grabbing---//
//Get an actor near the hand
AActor* AVRMotionController::GetActorNearHand()
{
	float nearestoverlap{ 10000.0f };
	AActor* nearestoverlappingactor{ nullptr };

	TArray <AActor*> actorarray;
	grabsphere->GetOverlappingActors(actorarray, AActor::StaticClass());
	for (const auto &eachactor : actorarray)
	{
		//!!!!!!!!!!!!!!!!!! implement some kind of check to filter actors that are interactible
		float tempoverlap{ (eachactor->GetActorLocation() - grabsphere->GetComponentLocation()).Size() };
		if ( tempoverlap < nearestoverlap)
		{
			nearestoverlappingactor = eachactor;
			nearestoverlap = tempoverlap;
		}
	}
	return nearestoverlappingactor;
}