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
#include "Runtime/Engine/Classes/Materials/MaterialInstanceDynamic.h"
#include "VRPickupObject.h"
#include "VRCharacterPawn.h"
#include "XRMotionControllerBase.h"

AVRMotionController::AVRMotionController()
{
	PrimaryActorTick.bCanEverTick = true;

	//---Left or Right hand---//
	AVRCharacterPawn* pawnref{ Cast<AVRCharacterPawn>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0)) };
	if (pawnref)
	{
		m_hand = pawnref->GetPawnHand();
	}

	//---Teleporter conditionals---//
	m_b_isteleactive = false;

	//---Teleport Rotation---//
	m_telerotation = FRotator{ 0.0f };

	//---Wrist-based orientation rotation value---//
	m_initialcontrollerrotation = FRotator{ 0.0f };

	/////////////////////////////////////////////////////////////////

	telelaunchvelocity = 900.0f;

	//Attached actor object
	attachedactor = nullptr;

	//Grip enum state
	gripstate = E_GRIPSTATE::GRIP_OPEN;

	b_isroomscale = false;

	b_shouldgrip = false;

	b_isvalidteledestination = false;
	b_isvalidpreviousframeteledestination = false;

	scene = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));

	motioncontroller = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MotionController"));
	motioncontroller->SetupAttachment(scene);
	//motioncontroller->SetTrackingSource(m_hand);
	handmesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HandMesh"));
	handmesh->SetupAttachment(motioncontroller);
	arcdirection = CreateDefaultSubobject<UArrowComponent>(TEXT("ArcDirection"));
	arcdirection->SetupAttachment(handmesh);
	arcspline = CreateDefaultSubobject<USplineComponent>(TEXT("ArcSpline"));
	arcspline->SetupAttachment(handmesh);
	arcspline->SetMobility(EComponentMobility::Movable);
	grabsphere = CreateDefaultSubobject<USphereComponent>(TEXT("GrabSphere"));
	grabsphere->SetupAttachment(handmesh);
	grabsphere->SetSphereRadius(10.0f);

	//Load arc end point
	arcendpoint = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArcEndPoint"));
	arcendpoint->SetupAttachment(scene);

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

	//

	static ConstructorHelpers::FObjectFinder<UStaticMesh> beam(TEXT("StaticMesh'/Game/VirtualReality/Meshes/BeamMesh.BeamMesh'"));
	if (beam.Object) {
		mesh_beamspline = beam.Object;
	}
	static ConstructorHelpers::FObjectFinder<UMaterial> spline(TEXT("Material'/Game/VirtualReality/Materials/M_SplineArcMat.M_SplineArcMat'"));
	if (spline.Object) {
		mat_spline = spline.Object;
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
		motioncontroller->MotionSource = FXRMotionControllerBase::LeftHandSourceId;
		handmesh->SetWorldScale3D(FVector(1.0f, 1.0f, -1.0f));
	}
	else
	{
		motioncontroller->MotionSource = FXRMotionControllerBase::RightHandSourceId;
	}

	grabsphere->OnComponentBeginOverlap.AddDynamic(this, &AVRMotionController::GrabSphereOnOverlap);
	handmesh->OnComponentHit.AddDynamic(this, &AVRMotionController::ControllerMeshOnHit);
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
		AActor* tempactor{ nullptr };
		if (GetActorNearHand(tempactor) != nullptr)
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

	//Handle teleport arc
	HandleTeleportationArc();

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
AActor*& AVRMotionController::GetActorNearHand(AActor *&actor)
{
	float nearestoverlap{ 10000.0f };

	TArray <AActor*> actorarray;
	grabsphere->GetOverlappingActors(actorarray, AActor::StaticClass());
	for (const auto &eachactor : actorarray)
	{
		//!!!!!!!!!!!!!!!!!! implement some kind of check to filter actors that are interactible
		float tempoverlap{ (eachactor->GetActorLocation() - grabsphere->GetComponentLocation()).Size() };
		if (tempoverlap < nearestoverlap)
		{
			actor = eachactor;
			nearestoverlap = tempoverlap;
		}
	}
	return actor;
}

//---Teleportation Arc---//
void AVRMotionController::HandleTeleportationArc()
{
	//Clear tele arc
	if (array_splinemeshes.IsValidIndex(0))
	{
		for (const auto &mesh : array_splinemeshes)
		{
			if (mesh)
			{
				mesh->DestroyComponent();
			}
		}
		array_splinemeshes.Empty();
	}
	arcspline->ClearSplinePoints(true);

	if (m_b_isteleactive)
	{
		FPredictProjectilePathParams params;
		params.StartLocation = FVector{ arcdirection->GetComponentLocation() };
		params.LaunchVelocity = FVector{ arcdirection->GetForwardVector() * telelaunchvelocity };
		params.bTraceWithCollision = true;
		params.ProjectileRadius = 0.0f;
		TArray<TEnumAsByte<EObjectTypeQuery>> teleboundry;
		teleboundry.Emplace(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
		params.ObjectTypes = teleboundry;
		params.bTraceComplex = false;
		params.SimFrequency = 30.0f;
		FPredictProjectilePathResult result;

		bool b_hit = UGameplayStatics::PredictProjectilePath(this, params, result);

		UNavigationSystemV1* navsystem = Cast<UNavigationSystemV1>(GetWorld()->GetNavigationSystem());
		bool b_pointhit;
		FNavLocation projectedlocation;
		if (navsystem)
		{
			b_pointhit = navsystem->ProjectPointToNavigation(result.HitResult.Location, projectedlocation, FVector{ 500.0f }, (ANavigationData*)0, FSharedConstNavQueryFilter{});
		}

		//Trace teleport destionation is success or not
		bool b_tracetelesuccess{ b_hit && b_pointhit };
		//Nav mesh target location
		FVector navmeshlocation{ projectedlocation };
		//Trace location
		FVector tracelocation{ result.HitResult.Location };
		//Trace points
		TArray<FVector> array_arcpoints;
		for (const auto &eachpoint : result.PathData)
		{
			array_arcpoints.Emplace(eachpoint.Location);
		}

		b_isvalidteledestination = b_tracetelesuccess;
		teleportcylinder->SetVisibility(b_isvalidteledestination, true);

		//Trace down to find a valid location for players to stand at, original NavMesh location is offset upwards, so we must find the actual floor
		TArray<TEnumAsByte<EObjectTypeQuery>> floorboundry;
		floorboundry.Emplace(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
		FHitResult outhit;
		UKismetSystemLibrary::LineTraceSingleForObjects(GetWorld(), navmeshlocation, FVector{ 0.0f, 0.0f, navmeshlocation.Z + -200.0f }, floorboundry, false, TArray<AActor*>(), EDrawDebugTrace::None, outhit, true);

		FVector newlocation{ outhit.bBlockingHit ? outhit.ImpactPoint : navmeshlocation };
		teleportcylinder->SetWorldLocation(newlocation, false, nullptr, ETeleportType::TeleportPhysics);

		//Rumble controller when a valid teleport location is found
		if ((b_isvalidteledestination && !b_isvalidpreviousframeteledestination) || (b_isvalidpreviousframeteledestination && !b_isvalidteledestination))
		{
			RumbleController(0.3);
		}

		b_isvalidpreviousframeteledestination = b_tracetelesuccess;

		//Create small stub line when it fails to find a teleport location
		if (!b_tracetelesuccess)
		{
			array_arcpoints.Empty();
			array_arcpoints.Emplace(arcdirection->GetComponentLocation());
			array_arcpoints.Emplace(arcdirection->GetComponentLocation() + (arcdirection->GetForwardVector() * 20.0f));
		}

		//Build spline from all trace points, generates tangets to build a smoothly curved spline mesh
		for (const auto &eachpoint : array_arcpoints)
		{
			arcspline->AddSplinePoint(eachpoint, ESplineCoordinateSpace::Local, true);
		}
		//Update the point type to create the curve
		arcspline->SetSplinePointType(array_arcpoints.Num(), ESplinePointType::CurveClamped, true);

		//Update spline
		for (int i{ 0 }; i < arcspline->GetNumberOfSplinePoints() - 2; i++)
		{
			USplineMeshComponent* splinemesh = NewObject<USplineMeshComponent>(arcspline);
			splinemesh->RegisterComponentWithWorld(GetWorld());
			splinemesh->SetMobility(EComponentMobility::Movable);
			splinemesh->SetStaticMesh(mesh_beamspline);
			UMaterialInstanceDynamic* dynamicMat = UMaterialInstanceDynamic::Create(mat_spline, NULL);
			splinemesh->bCastDynamicShadow = false;
			splinemesh->SetMaterial(0, dynamicMat);
			array_splinemeshes.Emplace(splinemesh);

			FVector pointlocationstart{ array_arcpoints[i] };
			FVector pointtangentstart{ arcspline->GetTangentAtSplinePoint(i, ESplineCoordinateSpace::Local) };
			FVector pointlocationend{ array_arcpoints[i + 1] };
			FVector pointtangentend{ arcspline->GetTangentAtSplinePoint(i + 1, ESplineCoordinateSpace::Local) };
			//Set tangents and position to slightly bend the cylinder, all cylinders combined create a smooth arc
			splinemesh->SetStartAndEnd(pointlocationstart, pointtangentstart, pointlocationend, pointtangentend, true);
		}

		arcendpoint->SetVisibility(b_tracetelesuccess && m_b_isteleactive, false);
		arcendpoint->SetWorldLocation(tracelocation, false, nullptr, ETeleportType::TeleportPhysics);
		FRotator HMDrotation{ 0.0f };
		FVector HMDposition{ 0.0f };
		UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(HMDrotation, HMDposition);
		arrow->SetWorldRotation(UKismetMathLibrary::ComposeRotators(m_telerotation, FRotator{ 0.0f, HMDrotation.Yaw, 0.0f }));
		roomscalemesh->SetWorldRotation(m_telerotation);
	}
}

//---Rumble controller on overlap with valid static mesh---//
void AVRMotionController::GrabSphereOnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (Cast<UStaticMeshComponent>(OtherComp)->IsSimulatingPhysics())
	{
		RumbleController(0.8f);
	}
}
void AVRMotionController::ControllerMeshOnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	RumbleController(UKismetMathLibrary::MapRangeClamped(NormalImpulse.Size(), 0.0f, 1500.0f, 0.0f, 0.8f));
}

//---Teleportation---//
void AVRMotionController::ActivateTele()
{
	m_b_isteleactive = true;
	teleportcylinder->SetVisibility(true, true);
	//Only show during teleport if room-scale is available
	roomscalemesh->SetVisibility(b_isroomscale, false);
	//Store rotation for later to compare roll value to support wrist-based orientation of the teleporter
	m_initialcontrollerrotation = motioncontroller->GetComponentRotation();
}
void AVRMotionController::DisableTele()
{
	if (m_b_isteleactive)
	{
		m_b_isteleactive = false;
		teleportcylinder->SetVisibility(false, true);
		arcendpoint->SetVisibility(false, false);
		roomscalemesh->SetVisibility(false, false);
	}
}

//---Grab and Release Actors---//
void AVRMotionController::GrabActor()
{
	b_shouldgrip = true;
	AActor* tempactor{ nullptr };
	if (GetActorNearHand(tempactor))
	{
		attachedactor = Cast<AVRPickupObject>(tempactor);
		if (attachedactor)
		{
			attachedactor->Pickup(motioncontroller);
			RumbleController(0.7f);
		}
	}
}
void AVRMotionController::ReleaseActor()
{
	b_shouldgrip = false;
	if (attachedactor)
	{
		if (attachedactor->GetRootComponent()->GetAttachParent() == motioncontroller)
		{
			attachedactor->Drop();
			RumbleController(0.2f);
		}
		attachedactor = nullptr;
	}
}

//---Getter---//
bool AVRMotionController::GetIsTeleActive()
{
	return m_b_isteleactive;
}
bool AVRMotionController::GetIsValidTeleDest()
{
	return b_isvalidteledestination;
}
FVector AVRMotionController::GetTeleDestLoc()
{
	FVector pos{ 0.0f };
	FRotator ori{ 0.0f };
	UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(ori, pos);

	UKismetMathLibrary::Quat_RotateVector(m_telerotation.Quaternion(), FVector{ pos.X, pos.Y, 0.0f });

	return FVector{ teleportcylinder->GetComponentLocation() - UKismetMathLibrary::Quat_RotateVector(m_telerotation.Quaternion(), FVector{ pos.X, pos.Y, 0.0f }) };
}
FRotator AVRMotionController::GetTeleDestRot()
{
	return m_telerotation;
}
FRotator AVRMotionController::GetInitControllerRot()
{
	return m_initialcontrollerrotation;
}
UMotionControllerComponent& AVRMotionController::GetMotionControllerComponent()
{
	return *motioncontroller;
}

//---Setter---//
void AVRMotionController::SetTeleRotation(FRotator newrot)
{
	m_telerotation = newrot;
}

//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("%d"), b_isvalidteledestination));
//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Back to normal"));