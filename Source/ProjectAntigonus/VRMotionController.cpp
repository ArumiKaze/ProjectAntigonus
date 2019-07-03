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
#include "Runtime/HeadMountedDisplay/Public/HeadMountedDisplayFunctionLibrary.h"
#include "Runtime/Engine/Classes/Components/SplineMeshComponent.h"
#include "Runtime/Engine/Classes/Components/SplineComponent.h"
#include "Runtime/Engine/Classes/Components/ArrowComponent.h"
#include "Runtime/Engine/Classes/Engine/World.h"
#include "Runtime/NavigationSystem/Public/NavigationSystem.h"

AVRMotionController::AVRMotionController()
	:AVRMotionController(EControllerHand::Left)
{
}
AVRMotionController::AVRMotionController(EControllerHand hand)
{
	telelaunchvelocity = 900.0f;

	//Attached actor object
	attachedactor = nullptr;

	//Grip enum state
	gripstate = E_GRIPSTATE::GRIP_OPEN;

	m_hand = hand;

	b_isroomscale = false;

	b_shouldgrip = false;

	b_isteleactive = false;
	b_isvalidteledestination = false;

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
	arcdirection = CreateDefaultSubobject<UArrowComponent>(TEXT("ArcDirection"));
	arcdirection->SetupAttachment(handmesh);
	arcspline = CreateDefaultSubobject<USplineComponent>(TEXT("ArcSpline"));
	arcspline->SetupAttachment(handmesh);
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

	//Update room scale
	UpdateRoomScaleOutline();

	//Only let hand collide with environment while gripping
	switch (gripstate)
	{
	case E_GRIPSTATE::GRIP_OPEN:
	case E_GRIPSTATE::GRIP_CANGRAB:
		handmesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case E_GRIPSTATE::GRIP_GRAB:
		handmesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		break;
	}
}

//---Room scale set up---//
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
void AVRMotionController::UpdateRoomScaleOutline()
{
	if (roomscalemesh->IsVisible())
	{
		FRotator hmdorientation{ 0.0f };
		FVector hmdposition{ 0.0f };
		UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(hmdorientation, hmdposition);
		
		FVector teleporttarget{ UKismetMathLibrary::Quat_UnrotateVector(FRotator{ 0.0f, hmdorientation.Yaw, 0.0f }.Quaternion() , FVector{ hmdposition.X, hmdposition.Y, 0.0f } / -1.0f) };
		roomscalemesh->SetRelativeLocation(teleporttarget, false, false);
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

//---Teleportation Arc---//
void AVRMotionController::HandleTeleportationArc()
{
	//Clear tele arc
	if (array_splinemeshes.IsValidIndex(0))
	{
		while (array_splinemeshes.IsValidIndex(0))
		{
			//This destroys the component but it does not remove it from the array.
			array_splinemeshes[0]->DestroyComponent();
			array_splinemeshes.RemoveAt(0);
		}
	}
	arcspline->ClearSplinePoints(true);

	//Trace teleport destionation
	bool tracetelesuccess{ false };
	TArray<FPredictProjectilePathPointData> array_tracepoints;
	FVector navmeshlocation{ 0.0f };
	FVector tracelocation{ 0.0f };
	if (b_isteleactive)
	{
		FPredictProjectilePathParams params;
		params.StartLocation = FVector{ arcdirection->GetComponentLocation() };
		params.LaunchVelocity = FVector{ arcdirection->GetForwardVector() * telelaunchvelocity };
		params.bTraceWithCollision = true;
		params.ProjectileRadius = 0.0f;
		TArray<TEnumAsByte<EObjectTypeQuery>> teleboundry;
		teleboundry.Add(EObjectTypeQuery::ObjectTypeQuery1);
		params.ObjectTypes = teleboundry;
		params.bTraceComplex = false;
		params.SimFrequency = 30.0f;

		FPredictProjectilePathResult result;

		bool b_hit{ UGameplayStatics::PredictProjectilePath(this, params, result) };

		UNavigationSystemV1* navsystem = Cast<UNavigationSystemV1>(GetWorld()->GetNavigationSystem());
		FNavLocation projectedlocation;
		bool b_projectpoint{ navsystem->ProjectPointToNavigation(result.HitResult.Location, projectedlocation, FVector{ 500.0f }, (ANavigationData*)0, 0) };
		

		tracetelesuccess = b_hit && b_projectpoint;
		array_tracepoints = result.PathData;
		navmeshlocation = projectedlocation;
		tracelocation = result.HitResult.Location;
	}

	b_isvalidteledestination = tracetelesuccess;

}