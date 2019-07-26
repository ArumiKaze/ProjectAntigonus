#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VRMotionController.generated.h"

UENUM(BlueprintType)
enum class E_GRIPSTATE : uint8
{
	GRIP_OPEN,
	GRIP_CANGRAB,
	GRIP_GRAB
};

UCLASS()
class PROJECTANTIGONUS_API AVRMotionController : public AActor
{
	GENERATED_BODY()

private:

	//---Room scale conditional---//
	bool m_b_isroomscale;

	//---Left or Right hand---//
	EControllerHand m_hand;
	
	//---Grip hand conditional---//
	bool m_b_shouldgrip;

	//---Controller Rumble Float Curve---//
	UPROPERTY()
	UHapticFeedbackEffect_Base* m_hapticbase_motioncontroller;

	//---Teleporter conditionals---//
	bool m_b_isteleactive;
	bool m_b_isvalidteledestination;

	//---Teleport Rotation---//
	FRotator m_telerotation;

	//---Wrist-based orientation rotation value---//
	FRotator m_initialcontrollerrotation;

	//---Actor that is attached to hand---//
	UPROPERTY()
	class AVRPickupObject* m_attachedactor;



	//---Room Scale---//
	void SetupRoomScaleOutline();

	//---Controller rumble---//
	void RumbleController(float intensity);

	//---Grabbing---//
	void GetActorNearHand(AActor*& actor);

	////////////////////////////////////////////////////////

	float telelaunchvelocity;

	bool b_isvalidpreviousframeteledestination;

	UPROPERTY()
	UStaticMesh* mesh_beamspline;
	UPROPERTY()
	UMaterial* mat_spline;

	UPROPERTY()
	UStaticMeshComponent* arcendpoint;

	UPROPERTY()
	TArray<class USplineMeshComponent*> array_splinemeshes;

	//---Room Scale---//
	void UpdateRoomScaleOutline();

	//---Teleportation Arc---//
	void HandleTeleportationArc();

	//---Rumble Events---//
	void GrabSphereOnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	void ControllerMeshOnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);

protected:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* m_component_scene;

	//---Hand---//
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class UMotionControllerComponent* m_component_motioncontroller;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* m_component_handmesh;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class UArrowComponent* m_component_arcdirection;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class USplineComponent* m_component_arcspline;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class USphereComponent* m_component_grabsphere;

	//---Teleport Cylinder---//
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent *m_component_teleportcylinder;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent *m_component_ring;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent *m_component_arrow;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent *m_component_roomscalemesh;

	//---SteamVRChaperone---//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	class USteamVRChaperoneComponent *m_component_steamvrchaperone;

	//---Begin Play---//
	virtual void BeginPlay() override;

	/////////////////////////////////////////////////////////////////////////////////////////////////

	//---Grip Enum---//
	UPROPERTY(BlueprintReadOnly)
	E_GRIPSTATE gripstate;

public:

	//---Grab and Release Actors---//
	void GrabActor();
	void ReleaseActor();

	//---Getter---//
	bool GetIsTeleActive();
	bool GetIsValidTeleDest();
	FVector GetTeleDestLoc();
	FRotator GetTeleDestRot();
	FRotator GetInitControllerRot();
	class UMotionControllerComponent& GetMotionControllerComponent();

	//---Setter--//
	void SetTeleRotation(FRotator newrot);

	///////////////////////////////////////////////////////////////////////////////

	//---Constructor---//
	AVRMotionController();

	//---Tick---//
	virtual void Tick(float DeltaTime) override;

	//---Teleportation---//
	void ActivateTele();
	void DisableTele();
};