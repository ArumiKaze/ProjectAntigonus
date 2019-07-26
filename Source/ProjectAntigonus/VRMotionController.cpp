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

	//---Room scale conditional---//
	m_b_isroomscale = false;

	//---Left or Right hand---//
	AVRCharacterPawn* pawnref{ Cast<AVRCharacterPawn>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0)) };
	if (pawnref)
	{
		m_hand = pawnref->GetPawnHand();
	}

	//---Grip hand conditional---//
	m_b_shouldgrip = false;

	//Load controller rumble float curve
	static ConstructorHelpers::FObjectFinder<UHapticFeedbackEffect_Base> hapticcurve(TEXT("HapticFeedbackEffect_Curve'/Game/VirtualReality/Mannequin/Character/HapticCurve_MotionController.HapticCurve_MotionController'"));
	if (hapticcurve.Object) {
		m_hapticbase_motioncontroller = hapticcurve.Object;
	}

	//---Teleporter conditionals---//
	m_b_isteleactive = false;
	m_b_isvalidteledestination = false;

	//---Teleport Rotation---//
	m_telerotation = FRotator{ 0.0f };

	//---Wrist-based orientation rotation value---//
	m_initialcontrollerrotation = FRotator{ 0.0f };

	//---Actor that is attached to hand---//
	m_attachedactor = nullptr;

	m_component_scene = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));

	//Load hand components
	m_component_motioncontroller = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MotionController"));
	m_component_motioncontroller->SetupAttachment(m_component_scene);
	m_component_handmesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HandMesh"));
	m_component_handmesh->SetupAttachment(m_component_motioncontroller);
	m_component_arcdirection = CreateDefaultSubobject<UArrowComponent>(TEXT("ArcDirection"));
	m_component_arcdirection->SetupAttachment(m_component_handmesh);
	m_component_arcspline = CreateDefaultSubobject<USplineComponent>(TEXT("ArcSpline"));
	m_component_arcspline->SetupAttachment(m_component_handmesh);
	m_component_arcspline->SetMobility(EComponentMobility::Movable);
	m_component_grabsphere = CreateDefaultSubobject<USphereComponent>(TEXT("GrabSphere"));
	m_component_grabsphere->SetupAttachment(m_component_handmesh);
	m_component_grabsphere->SetSphereRadius(10.0f);

	//Load Teleport Cylinder meshes
	m_component_teleportcylinder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TeleportCylinder"));
	m_component_teleportcylinder->SetupAttachment(m_component_scene);
	m_component_ring = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ring"));
	m_component_ring->SetupAttachment(m_component_teleportcylinder);
	m_component_arrow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Arrow"));
	m_component_arrow->SetupAttachment(m_component_teleportcylinder);
	m_component_roomscalemesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RoomScaleMesh"));
	m_component_roomscalemesh->SetupAttachment(m_component_arrow);

	//Load SteamVR Chaperone
	m_component_steamvrchaperone = CreateDefaultSubobject<USteamVRChaperoneComponent>(TEXT("SteamVRChaperone"));

	/////////////////////////////////////////////////////////////////

	telelaunchvelocity = 900.0f;

	//Grip enum state
	gripstate = E_GRIPSTATE::GRIP_OPEN;

	b_isvalidpreviousframeteledestination = false;

	//Load arc end point
	arcendpoint = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArcEndPoint"));
	arcendpoint->SetupAttachment(m_component_scene);

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

//---Begin Play---//
void AVRMotionController::BeginPlay()
{
	Super::BeginPlay();

	//Set up room scale
	SetupRoomScaleOutline();

	//Hide teleport cylinder until activation
	m_component_teleportcylinder->SetVisibility(false, true);
	m_component_roomscalemesh->SetVisibility(false, true);

	//Invert scale on hand mesh to create left hand
	if (m_hand == EControllerHand::Left)
	{
		m_component_motioncontroller->MotionSource = FXRMotionControllerBase::LeftHandSourceId;
		m_component_handmesh->SetWorldScale3D(FVector(1.0f, 1.0f, -1.0f));
	}
	else
	{
		m_component_motioncontroller->MotionSource = FXRMotionControllerBase::RightHandSourceId;
	}

	m_component_grabsphere->OnComponentBeginOverlap.AddDynamic(this, &AVRMotionController::GrabSphereOnOverlap);
	m_component_handmesh->OnComponentHit.AddDynamic(this, &AVRMotionController::ControllerMeshOnHit);
}

void AVRMotionController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//Update animation of hand
	if (m_attachedactor || m_b_shouldgrip)
	{
		gripstate = E_GRIPSTATE::GRIP_GRAB;
	}
	else
	{
		AActor* tempactor{ nullptr };
		GetActorNearHand(tempactor);
		if (tempactor)
		{
			gripstate = E_GRIPSTATE::GRIP_CANGRAB;
		}
		else
		{
			if (m_b_shouldgrip)
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
		m_component_handmesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case E_GRIPSTATE::GRIP_GRAB:
		m_component_handmesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		break;
	}
}

//---Room Scale---//
//Set up
void AVRMotionController::SetupRoomScaleOutline()
{
	FVector OutRectCenter{ 0.0f };
	FRotator OutRectRotation{ 0.0f };
	float OutSideLengthX{ 0.0f };
	float OutSideLengthY{ 0.0f };
	UKismetMathLibrary::MinimumAreaRectangle(this, m_component_steamvrchaperone->GetBounds(), FVector{ 0.0f, 0.0f, 0.0f }, OutRectCenter, OutRectRotation, OutSideLengthX, OutSideLengthY, false);

	//Measure Chaperone, default to 100x100 if roomscale is not used
	m_b_isroomscale = UKismetMathLibrary::BooleanNAND(UKismetMathLibrary::NearlyEqual_FloatFloat(OutSideLengthX, 100.0f, 0.01f), UKismetMathLibrary::NearlyEqual_FloatFloat(OutSideLengthY, 100.0f, 0.01f));
	if (m_b_isroomscale)
	{
		//Setup room-scale mesh (1x1x1 units in size by default) to the size of the room-scale dimensions
		float chaperonemeshheight{ 70.0f };
		m_component_roomscalemesh->SetWorldScale3D(FVector{ OutSideLengthX, OutSideLengthY, chaperonemeshheight });
		m_component_roomscalemesh->SetRelativeRotation(OutRectRotation);
	}
}
//Update
void AVRMotionController::UpdateRoomScaleOutline()
{
	if (m_component_roomscalemesh->IsVisible())
	{
		FRotator hmdorientation{ 0.0f };
		FVector hmdposition{ 0.0f };
		UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(hmdorientation, hmdposition);

		FVector teleporttarget{ UKismetMathLibrary::Quat_UnrotateVector(FRotator{ 0.0f, hmdorientation.Yaw, 0.0f }.Quaternion() , FVector{ hmdposition.X, hmdposition.Y, 0.0f } / -1.0f) };
		m_component_roomscalemesh->SetRelativeLocation(teleporttarget, false, false);
	}
}

void AVRMotionController::RumbleController(float intensity)
{
	UGameplayStatics::GetPlayerController(GetWorld(), 0)->PlayHapticEffect(m_hapticbase_motioncontroller, m_hand, intensity, false);
}

//---Grabbing---//
//Get an actor near the hand
void AVRMotionController::GetActorNearHand(AActor *&actor)
{
	float nearestoverlap{ 10000.0f };

	TArray <AActor*> actorarray;
	m_component_grabsphere->GetOverlappingActors(actorarray, AActor::StaticClass());
	for (const auto &eachactor : actorarray)
	{
		//!!!!!!!!!!!!!!!!!! implement some kind of check to filter actors that are interactible
		float tempoverlap{ (eachactor->GetActorLocation() - m_component_grabsphere->GetComponentLocation()).Size() };
		if (tempoverlap < nearestoverlap)
		{
			actor = eachactor;
			nearestoverlap = tempoverlap;
		}
	}
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
	m_component_arcspline->ClearSplinePoints(true);

	if (m_b_isteleactive)
	{
		FPredictProjectilePathParams params;
		params.StartLocation = FVector{ m_component_arcdirection->GetComponentLocation() };
		params.LaunchVelocity = FVector{ m_component_arcdirection->GetForwardVector() * telelaunchvelocity };
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

		m_b_isvalidteledestination = b_tracetelesuccess;
		m_component_teleportcylinder->SetVisibility(m_b_isvalidteledestination, true);

		//Trace down to find a valid location for players to stand at, original NavMesh location is offset upwards, so we must find the actual floor
		TArray<TEnumAsByte<EObjectTypeQuery>> floorboundry;
		floorboundry.Emplace(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
		FHitResult outhit;
		UKismetSystemLibrary::LineTraceSingleForObjects(GetWorld(), navmeshlocation, FVector{ 0.0f, 0.0f, navmeshlocation.Z + -200.0f }, floorboundry, false, TArray<AActor*>(), EDrawDebugTrace::None, outhit, true);

		FVector newlocation{ outhit.bBlockingHit ? outhit.ImpactPoint : navmeshlocation };
		m_component_teleportcylinder->SetWorldLocation(newlocation, false, nullptr, ETeleportType::TeleportPhysics);

		//Rumble controller when a valid teleport location is found
		if ((m_b_isvalidteledestination && !b_isvalidpreviousframeteledestination) || (b_isvalidpreviousframeteledestination && !m_b_isvalidteledestination))
		{
			RumbleController(0.3);
		}

		b_isvalidpreviousframeteledestination = b_tracetelesuccess;

		//Create small stub line when it fails to find a teleport location
		if (!b_tracetelesuccess)
		{
			array_arcpoints.Empty();
			array_arcpoints.Emplace(m_component_arcdirection->GetComponentLocation());
			array_arcpoints.Emplace(m_component_arcdirection->GetComponentLocation() + (m_component_arcdirection->GetForwardVector() * 20.0f));
		}

		//Build spline from all trace points, generates tangets to build a smoothly curved spline mesh
		for (const auto &eachpoint : array_arcpoints)
		{
			m_component_arcspline->AddSplinePoint(eachpoint, ESplineCoordinateSpace::Local, true);
		}
		//Update the point type to create the curve
		m_component_arcspline->SetSplinePointType(array_arcpoints.Num(), ESplinePointType::CurveClamped, true);

		//Update spline
		for (int i{ 0 }; i < m_component_arcspline->GetNumberOfSplinePoints() - 2; i++)
		{
			USplineMeshComponent* splinemesh = NewObject<USplineMeshComponent>(m_component_arcspline);
			splinemesh->RegisterComponentWithWorld(GetWorld());
			splinemesh->SetMobility(EComponentMobility::Movable);
			splinemesh->SetStaticMesh(mesh_beamspline);
			UMaterialInstanceDynamic* dynamicMat = UMaterialInstanceDynamic::Create(mat_spline, NULL);
			splinemesh->bCastDynamicShadow = false;
			splinemesh->SetMaterial(0, dynamicMat);
			array_splinemeshes.Emplace(splinemesh);

			FVector pointlocationstart{ array_arcpoints[i] };
			FVector pointtangentstart{ m_component_arcspline->GetTangentAtSplinePoint(i, ESplineCoordinateSpace::Local) };
			FVector pointlocationend{ array_arcpoints[i + 1] };
			FVector pointtangentend{ m_component_arcspline->GetTangentAtSplinePoint(i + 1, ESplineCoordinateSpace::Local) };
			//Set tangents and position to slightly bend the cylinder, all cylinders combined create a smooth arc
			splinemesh->SetStartAndEnd(pointlocationstart, pointtangentstart, pointlocationend, pointtangentend, true);
		}

		arcendpoint->SetVisibility(b_tracetelesuccess && m_b_isteleactive, false);
		arcendpoint->SetWorldLocation(tracelocation, false, nullptr, ETeleportType::TeleportPhysics);
		FRotator HMDrotation{ 0.0f };
		FVector HMDposition{ 0.0f };
		UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(HMDrotation, HMDposition);
		m_component_arrow->SetWorldRotation(UKismetMathLibrary::ComposeRotators(m_telerotation, FRotator{ 0.0f, HMDrotation.Yaw, 0.0f }));
		m_component_roomscalemesh->SetWorldRotation(m_telerotation);
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
	m_component_teleportcylinder->SetVisibility(true, true);
	//Only show during teleport if room-scale is available
	m_component_roomscalemesh->SetVisibility(m_b_isroomscale, false);
	//Store rotation for later to compare roll value to support wrist-based orientation of the teleporter
	m_initialcontrollerrotation = m_component_motioncontroller->GetComponentRotation();
}
void AVRMotionController::DisableTele()
{
	if (m_b_isteleactive)
	{
		m_b_isteleactive = false;
		m_component_teleportcylinder->SetVisibility(false, true);
		arcendpoint->SetVisibility(false, false);
		m_component_roomscalemesh->SetVisibility(false, false);
	}
}

//---Grab and Release Actors---//
void AVRMotionController::GrabActor()
{
	m_b_shouldgrip = true;
	AActor* tempactor{ nullptr };
	GetActorNearHand(tempactor);
	if (tempactor)
	{
		m_attachedactor = Cast<AVRPickupObject>(tempactor);
		if (m_attachedactor)
		{
			m_attachedactor->Pickup(m_component_motioncontroller);
			RumbleController(0.7f);
		}
	}
}
void AVRMotionController::ReleaseActor()
{
	m_b_shouldgrip = false;
	if (m_attachedactor)
	{
		if (m_attachedactor->GetRootComponent()->GetAttachParent() == m_component_motioncontroller)
		{
			m_attachedactor->Drop();
			RumbleController(0.2f);
		}
		m_attachedactor = nullptr;
	}
}

//---Getter---//
bool AVRMotionController::GetIsTeleActive()
{
	return m_b_isteleactive;
}
bool AVRMotionController::GetIsValidTeleDest()
{
	return m_b_isvalidteledestination;
}
FVector AVRMotionController::GetTeleDestLoc()
{
	FVector pos{ 0.0f };
	FRotator ori{ 0.0f };
	UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(ori, pos);

	return FVector{ m_component_teleportcylinder->GetComponentLocation() - UKismetMathLibrary::Quat_RotateVector(m_telerotation.Quaternion(), FVector{ pos.X, pos.Y, 0.0f }) };
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
	return *m_component_motioncontroller;
}

//---Setter---//
void AVRMotionController::SetTeleRotation(FRotator newrot)
{
	m_telerotation = newrot;
}

//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("%d"), b_isvalidteledestination));
//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Back to normal"));