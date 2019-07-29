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
	bool m_b_isvalidpreviousframeteledestination;

	//---Teleport Rotation---//
	FRotator m_telerotation;

	//---Wrist-based orientation rotation value---//
	FRotator m_initialcontrollerrotation;

	//---Actor that is attached to hand---//
	UPROPERTY()
	class AVRPickupObject* m_attachedactor;

	//---Array of spline mesh components---//
	UPROPERTY()
	TArray<class USplineMeshComponent*> m_array_splinemeshes;

	//---Speed of tele arc launch velocity---//
	float m_telelaunchvelocity;

	//---Beam spline mesh and mat---//
	UPROPERTY()
	UStaticMesh* m_mesh_beamspline;
	UPROPERTY()
	UMaterial* m_mat_spline;





	//---Room Scale---//
	void SetupRoomScaleOutline();	//Setup
	void UpdateRoomScaleOutline();	//Update

	//---Controller rumble---//
	void RumbleController(float intensity);

	//---Rumble Events---//
	void GrabSphereOnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	void ControllerMeshOnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);

	//---Grabbing---//
	void GetActorNearHand(AActor*& actor);

	//---Teleportation Arc---//
	void HandleTeleportationArc();

protected:

	//---Grip Enum---//
	UPROPERTY(BlueprintReadOnly)
	E_GRIPSTATE m_enum_gripstate;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	USceneComponent* m_component_scene;

	//---Hand---//
	UPROPERTY(VisibleAnywhere, Category = "Components")
	class UMotionControllerComponent* m_component_motioncontroller;
	UPROPERTY(VisibleAnywhere, Category = "Components")
	USkeletalMeshComponent* m_component_handmesh;
	UPROPERTY(VisibleAnywhere, Category = "Components")
	class UArrowComponent* m_component_arcdirection;
	UPROPERTY(VisibleAnywhere, Category = "Components")
	class USplineComponent* m_component_arcspline;
	UPROPERTY(VisibleAnywhere, Category = "Components")
	class USphereComponent* m_component_grabsphere;

	//---Beam spline arc end point---//
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent *m_component_arcendpoint;

	//---Teleport Cylinder---//
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent *m_component_teleportcylinder;
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent *m_component_ring;
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent *m_component_arrow;
	UPROPERTY(VisibleAnywhere, Category = "Components")
	UStaticMeshComponent *m_component_roomscalemesh;

	//---SteamVRChaperone---//
	UPROPERTY(VisibleAnywhere, Category = "Components")
	class USteamVRChaperoneComponent *m_component_steamvrchaperone;

	//---Begin Play---//
	virtual void BeginPlay() override;

public:

	//---Constructor---//
	AVRMotionController();

	//---Tick---//
	virtual void Tick(float DeltaTime) override;

	//---Grab and Release Actors---//
	void GrabActor();
	void ReleaseActor();

	//---Teleportation---//
	void ActivateTele();
	void DisableTele();

	//---Getter---//
	bool GetIsTeleActive();
	bool GetIsValidTeleDest();
	FVector GetTeleDestLoc();
	FRotator GetTeleDestRot();
	FRotator GetInitControllerRot();
	class UMotionControllerComponent& GetMotionControllerComponent();

	//---Setter--//
	void SetTeleRotation(FRotator newrot);
};